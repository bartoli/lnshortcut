"""
This tool:
 - Updates max_htlc_msat value for channels from channel local balance
 - Applies our fee strategy
 - is not constantly running, but must be called (by cron for example) at the frequency you want updates

On max_htlc_msat update:
Why update:
 - Foreign nodes do not know our channel balance. It helps privacy, but makes every routing node
   not know if a route will fail or not because a channel has enough balance on the needed side.
   After a certain number of failed HTLC, other nodes will probably blacklist that channel as non
   routing one, and stop trying to route from it, even when we rebalance it. Or even close it if
   it is a channel to them. If we advertise a max haltc value that will not fail, they will avoid
   wasting a payment attempt, and avoid getting a payment error
 - Globally, we think it is a useful informatoon for he network for efficient routes. And our 
   channel advisor makes use of it to suggest the next good peer
How we update max_htlc_msat value:
 - We want max_htlc to bear neer the local balance value.
 - We need to always have a certain reserve in the channel (default=25000sats)
 - We cannot set max_htlc_msat < min_htlc_msat
 - The RPC call to change channel policy params takes multiple params, and 0 value can mean
  'do not change the value'. Attention must be put into not wrongly setting max_htlc_msat value to 0

Fee update policy
We choose a policy with a fixed fee.
Base Fee is 0, and fee rate is 1 by default. We chose this to be the smallest non null fee to chose 
route quantity instead of little routes with higher fees, for more versatility of the node.
All channels have this fee. Except for channels with >=90% of local capacity. Those are set to a fee
rate of 0 to encourage routes in the other drection. We will not earn fees on this direction, but
each route in this directon alloaw one more route in the directon that collects fees.
On min htlc value, on most channels, it is set to 1000 sats, so all route gieve back at least 1 msat.
This is because any route has the potential to trigger a force close of a channel, so it must have
a cost!
For channels with >=90 local balance, we lower back min_htlc value to 0 to also encourage
traffic in this direction.

On channel policy update frequency
We must not spam the network with too many updates. At best, it adds processing load to every node
on the network for each update. At worst, it's another reason to be blacklisted by some nodes.
We choose to do one channel update per hour (for a single channel).
So we have a prioritization scheme for which chanel will be updated.
Criteria for channel max_htlc_msat updates
 - max_htlc_msat increases have priority on decreases, as they potentially allow new routes earlier,
   while decrease 'only' inform earlier tha troutes cannot anymore pass
 - bigger max_htlc_msat delta updates have priority on the ones updating for a small value change
Criteria for fee updates:
 - the need for a fee decrease means liquidity did not flow anymore, so those have more priority
   than updates that increase fees. For this same reason, updates for fee decreases have priority on updates for max_htlc_value
 - Updates for max_htlc decrease have the least interest for us, so we treat them after udpates for fees increase
 If achannel has been selected for update, we try to merge any fees/max_htlx/min_htlc upodates it
 may need as this will not trigger more update messages

Channel update failures
Channel state is queried after each update, to check that the update actually succeeded. If not, to
avoid endless loop on trying to update this channel, it is locked for updates for the next as many
cycles as you have channels (N channels = N hours before we try again to update it). And the update
of the next channel on th epriorty list is updated to not waste this update cycle.
This state is saved by creating flag files named chanid_lock_timestamp in the app settings directory.
YOu can delete them to ignore the locks for next cycle
"""

import json
import subprocess

"""
This class is the interface to lncli It wraps the following commands to to what the script needs:
 - FILL ME
Point it to your lncli executable, of a lncli wrapper that calls the same commands on lncli, if
you want to make sure it will not do anything else with your node
"""
class LnCliWrapper:
  def __init__(self, lncliCmd):
    #Set this to a command to run your lncli or lncli wrapper
    self.lncliCmd = lncliCmd
    self.pubkey = self.getNodePubkey()
    print("Node pubkey: "+self.pubkey)

  #Base method that runs lncli with the given command/arguments on th ecommand line, 
  #and returns exit code / output
  def _runCmd(self, command : str) -> tuple[int, str, str]: 
    cmd = self.lncliCmd + " " + command
    ret = subprocess.run(cmd.split(" "), capture_output=True)
    return (ret.returncode, ret.stdout, ret.stderr)

  def updateChanPolicy(self, maxHtlc: int, channelPoint: str, baseFeeMsat: int, feerateMsat: int, chosenTld: int) -> tuple[int, str, str]:
    return self._runCmd("updatechanpolicy" +
    " --max_htlc_msat"   + str(maxHtlc*1000) +
    " --chan_point"      + str(channelPoint) +
    " --base_fee_msat"   + str(baseFeeMsat) +
    " --fee_rate_ppm"    + str(feerateMsat) +
    " --time_lock_delta" + str(chosenTld))
  
  def listChanInfo(self):
    return self._runCmd("listchannels")
  def getChanInfo(self, chan_id:str):
    print(chan_id)
    return self._runCmd("getchaninfo --chan_id " + chan_id)
  def getNodePubkey(self):
    (ret,out,err) = self._runCmd("getinfo")
    #print(out)
    in_data = json.loads(out)
    return in_data["identity_pubkey"]

class ChanelPolicy:
  min_htlc_msat = int()
  max_htlc_msat = int()
  tld = int()
  base_fee = int()
  fee_rate = int()
  in_fee_rate = int()

  


lncli = LnCliWrapper("/softs/lnd/current/lncli")

def LoadChanData():
  (ret, out,err) = lncli.listChanInfo()
  #print(out)
  in_data = json.loads(out)
  channels = in_data["channels"]
  chan_data = []
  for channel in channels:
    chan_id = channel["scid"]
    channel_data = dict()
    channel_data["peer_alias"] = channel["peer_alias"]
    channel_data["capacity"] = int(channel["capacity"])
    channel_data["local_balance"] = int(channel["local_balance"])
    channel_data["remote_balance"] = int(channel["remote_balance"])
    channel_data["channel_point"] = channel["chan_point" if "chan_point" in channel else "channel_point"]
    channel_data["channel_id"] = chan_id
    print(chan_id)
    #extra info
    (ret2, out2, err2) = lncli.getChanInfo(chan_id)
    if(len(out2)==0):
      print("Error for "+chan_id+". Skipping")
      if len(err2)>0:
        print(err2)
      continue
    print(out2)
    in_data2 = json.loads(out2)
    local_policy_key = "node1_policy"  if (in_data2["node1_pub"] == lncli.pubkey) else "node2_policy"
    policy = in_data2[local_policy_key]
    channel_data["min_htlc_msat"] = int(policy["min_htlc"])
    channel_data["fee_base_msat"] = int(policy["fee_base_msat"])
    channel_data["fee_rate_mmsat"] = int(policy["fee_rate_milli_msat"])
    channel_data["in_fee_rate_mmsat"] = int(policy["inbound_fee_rate_milli_msat"])
    channel_data["max_htlc_msat"] = int(policy["max_htlc_msat"])
    channel_data["time_lock_delta"] = int(policy["time_lock_delta"])
    #print(channel_data)
    chan_data.append(channel_data)
  return chan_data

def update_command(chan_point, bf, fr, in_bf, in_fr, tld, min_htlc, max_htlc):
  """
  USAGE:
   lncli updatechanpolicy [command options] base_fee_msat fee_rate time_lock_delta [--max_htlc_msat=N] [channel_point]

  OPTIONS:
   --base_fee_msat value          the base fee in milli-satoshis that will be charged for each forwarded HTLC, regardless of payment size (default: 0)
   --fee_rate value               the fee rate that will be charged proportionally based on the value of each forwarded HTLC, the lowest possible rate is 0 with a granularity of 0.000001 (millionths). Can not be set at the same time as fee_rate_ppm
   --fee_rate_ppm value           the fee rate ppm (parts per million) that will be charged proportionally based on the value of each forwarded HTLC, the lowest possible rate is 0 with a granularity of 0.000001 (millionths). Can not be set at the same time as fee_rate (default: 0)
   --inbound_base_fee_msat value  the base inbound fee in milli-satoshis that will be charged for each forwarded HTLC, regardless of payment size. Its value must be zero or negative - it is a discount for using a particular incoming channel. Note that forwards will be rejected if the discount exceeds the outbound fee (forward at a loss), and lead to penalization by the sender (default: 0)
   --inbound_fee_rate_ppm value   the inbound fee rate that will be charged proportionally based on the value of each forwarded HTLC and the outbound fee. Fee rate is expressed in parts per million and must be zero or negative - it is a discount for using a particular incoming channel.Note that forwards will be rejected if the discount exceeds the outbound fee (forward at a loss), and lead to penalization by the sender (default: 0)
   --time_lock_delta value        the CLTV delta that will be applied to all forwarded HTLCs (default: 0)
   --min_htlc_msat value          if set, the min HTLC size that will be applied to all forwarded HTLCs. If unset, the min HTLC is left unchanged (default: 0)
   --max_htlc_msat value          if set, the max HTLC size that will be applied to all forwarded HTLCs. If unset, the max HTLC is left unchanged (default: 0)
   --chan_point value             the channel which this policy update should be applied to. If nil, the policies for all channels will be updated. Takes the form of txid:output_index
  """
  command = "updatechanpolicy --inbound_base_fee_msat="+str(in_bf) + " --inbound_fee_rate_ppm="+str(in_fr) \
    + " --fee_rate_ppm="+str(fr) + " --time_lock_delta="+str(tld) \
    + " --min_htlc_msat="+str(min_htlc) + " --max_htlc_msat="+str(int(max_htlc)) \
    +" "+str(bf)+" "+chan_point
  return command

htlc_increase_queue = dict()
htlc_decrease_queue = dict()
fee_decrease_queue = dict()
fee_increase_queue = dict()
work_per_channel = dict()

#Load current channel states
channels_data = LoadChanData()
for channel in channels_data:
  #Optimize channel outbound policy
    
  local = channel["local_balance"]
  cap = channel["capacity"]
  reserve = cap/100
  
  
  min_htlc_msat = 1000#000
  #Make max_htlc value follow local balance-reserve to avoid promising routing what we can't
  #max_htlc can also never be lower than the min_htlc
  wanted_max_htlc = max(local-reserve, min_htlc_msat/1000)
  
  size_multiplier = int((cap+500000)/1000000)
  #print(size_multiplier)
  feerate_ppm = size_multiplier
  in_feerate_ppm = 0

  if local<reserve:
    #if local balance  is too litle, signal we can't route outbound anymore
    #smallest non zero values, because for the api, 0 means 'do not change the value'
    #with the requirement that max_htlc>=min_htlc
    #Improvement: We could disable our side of the channel and keep the policy instead?
    wanted_max_htlc = 1
    min_htlc_msat = 1
  if float(local)/cap > 0.9:
    #above 90% of local balance, reduce outbound fee to encourage flow in the other direction
    feerate_ppm = 0
    #And remove limit on min transaction size. We won't get any fee, so the min fee that ensured every route was not free is useless
    #min_htlc_msat = 0
  if float(local)/cap < 0.1:
    #bellow 10% cap, sink channel should offer inbound discounts
    in_feerate_ppm = -20 

  #Now register needed work for this channel
  chan_id = channel["channel_id"]
  work_per_channel[chan_id] = []
  #generated channel update command for each channel, indexed by chan_id
  command_per_channel = []
  max_htlc_delta = wanted_max_htlc*1000 - channel["max_htlc_msat"]
  if max_htlc_delta>0:
    htlc_increase_queue[chan_id] = max_htlc_delta
    work_per_channel[chan_id].append("Increase max_htlc from "+str(channel["max_htlc_msat"]/1000)+" to "+str(wanted_max_htlc))
  elif max_htlc_delta<0:
    htlc_decrease_queue[chan_id] = max_htlc_delta
    work_per_channel[chan_id].append("Decrease max_htlc from "+str(channel["max_htlc_msat"]/1000)+" to "+str(wanted_max_htlc))

  if feerate_ppm > channel["fee_rate_mmsat"]:
    fee_increase_queue[chan_id] = feerate_ppm
    work_per_channel[chan_id].append("Reset to nominal fee")
  elif feerate_ppm < channel["fee_rate_mmsat"]:
    fee_decrease_queue[chan_id] = feerate_ppm
    work_per_channel[chan_id].append("Cancel feerate")
  elif in_feerate_ppm != channel["in_fee_rate_mmsat"]:
    fee_decrease_queue[chan_id] = feerate_ppm
    work_per_channel[chan_id].append("Update inbound feerate")

  print(channel["peer_alias"]+": Local="+str(local)+"("+str(float(local)*100/cap)+"%) feerate="+str(channel["fee_rate_mmsat"])+ \
        " infeerate="+str(channel["in_fee_rate_mmsat"])+" max_htlc="+str(wanted_max_htlc))
  if(len(work_per_channel[chan_id])>0):
    print(work_per_channel[chan_id])
    cmd = (update_command(channel["chan_point" if "chan_point" in channel else "channel_point"],0, feerate_ppm, 0, in_feerate_ppm, channel["time_lock_delta"], min_htlc_msat, wanted_max_htlc*1000))
    print(cmd)
    (code, out,err) = lncli._runCmd(cmd)
    #print(out)
    #print(err)


#Calculate optimum wanted config for each channel





    
