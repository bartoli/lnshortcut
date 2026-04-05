#include <Config.hpp>
#include <NetworkSummary.hpp>

Config::Config()
{
    min_chan_size_db.insert( 3999999, "032ae3168ba52314da581d6b6693c562b437a9cf805933d4d69e7801547e07302e");// LQWD-France
    min_chan_size_db.insert( 4000000, "03423790614f023e3c0cdaa654a3578e919947e4c3a14bf5044e7c787ebd11af1a");// SunnySarah
    min_chan_size_db.insert( 4000000, "033b63e4a9931dc151037acbce12f4f8968c86f5655cf102bbfa85a26bd4adc6d9");// Garlic
    min_chan_size_db.insert( 4000000, "033d8656219478701227199cbd6f670335c8d408a92ae88b962c49d4dc0e83e025");// bfx-lnd0
    min_chan_size_db.insert( 4000000, "03cde60a6323f7122d5178255766e38114b4722ede08f7c9e0c5df9b912cc201d6");// bfx-lnd1
    min_chan_size_db.insert( 4000000, "0378a5312ca36773971df43f833b571d024fd7d5d216fbdf7e241b710714ef1be1");// OneKey
    min_chan_size_db.insert( 4206921, "039e05e271f537cfa1c060d2364b960b85bd509ac89bae524e4a01948a07b3e8d1");// LnockOnWood
    min_chan_size_db.insert( 5000000, "035e4ff418fc8b5554c5d9eea66396c227bd429a3251c8cbc711002ba215bfc226");// WalletofSatoshi
    min_chan_size_db.insert( 5000000, "033d9e73a183c9714545f292875fb90c4372bddc9c2cc302b265d15e7969a5ed60");// NordicRails
    min_chan_size_db.insert( 5000000, "03271338633d2d37b285dae4df40b413d8c6c791fbee7797bc5dc70812196d7d5c");// lnmarkets.com
    min_chan_size_db.insert( 6000000, "0326e692c455dd554c709bbb470b0ca7e0bb04152f777d1445fd0bf3709a2833a3");// LnCapital
    min_chan_size_db.insert( 6969420, "03c5528c628681aa17ab9e117aa3ee6f06c750dfb17df758ecabcd68f1567ad8c1");// ⚡G-Spot-21_69_420⚡
    min_chan_size_db.insert(10000000, "031015a7839468a3c266d662d5bb21ea4cea24226936e2864a7ca4f2c3939836e0");// Breez
    min_chan_size_db.insert(10000000, "03037dc08e9ac63b82581f79b662a4d0ceca8a8ca162b1af3551595b8f2d97b70a");// river Financial 1
    min_chan_size_db.insert(10000000, "03dc686001f9b1ff700dfb8917df70268e1919433a535e1fb0767c19223509ab57");// cyberhub
    min_chan_size_db.insert(10000000, "0299797daa21faa6e4151517b286c41d4c7a8cb21fbfba59ca3bcd21a9f8392bd4");// coinhodler04108
    min_chan_size_db.insert(16777215, "03d06758583bb5154774a6eb221b1276c9e82d65bbaceca806d90e20c108f4b1c7");// yalls.org TOR
    min_chan_size_db.insert(16777215, "0288be11d147e1525f7f234f304b094d6627d2c70f3313d7ba3696887b261c4447");// yalls.org clearnet

    //no min chan size, but doe snot accept channels (some breez intermediate node)
    min_chan_size_db.insert(20000000, "02c811e575be2df47d8b48dab3d3f1c9b0f6e16d0d40b5ed78253308fc2bd7170d");// BreezC
    //coop closed after a few days of nice traffic. Little number of channels. does not want peers?
    //min_chan_size_db.insert(20000000, "03c96bdfcb9d4c107385c96f1b60a3910f4f28e679fb119d8ee5e243217f717bd3");// wallet-of-UXUY
    //tried multiple times, force coses each time. lightning terminal says 'channel stability' is bad
    min_chan_size_db.insert(20000000, "02ddb472c8edf0624dec5544507b705aae67d42a6cc2db133ba7e843fd5ce90135");// AfricaFreeRouting
    //can not connect to public tor address
    min_chan_size_db.insert(20000000, "02bb5e47ee26aca1654ce092d9d06eac15d953210c737f2220e2d7e54c4f108677");// ConnectsBack


    lns_pubkey = "02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683";

    //0-based hop count for list of candidates
    candidates_depth = 1;
}

bool Config::excludesNodeForRouting(const Node& node) const
{
  if(node.edges.empty())
      return true;
  if(node.totalCapacity > 50*100000000ULL)
      return true;

  return false;
};

bool Config::excludesNodeAsEndPoint(const Node& node, const NetworkSummary& network) const
{
    const Config& cfg(*this);

    /*if(node.pubKey=="037ba9c3a14eb7966bccf2ceb5606456dd13c7523dfdeb754137cc98db2b66e4d8")
        puts("Test");*/

    bool reachable_on_clearnet = node.clearnet && cfg.clearnetNodes;
    bool reachable_on_tor = node.tor && cfg.torNodes;
    if (!(reachable_on_clearnet || reachable_on_tor))
            return true;

    if(cfg.zbfEndpoints && ! node.isZbf)
         return true;

    //node must have at least two channels of at least wanted endpoint capacity
    int channels_with_wanted_final_cap = 0;
    for(int ie=0, ecnt = node.edges.size(); ie<ecnt; ++ie)
    {
        const Edge& edge = network.edges[node.edges[ie]];
        if(edge.capacity >= cfg.minEndpointCap)
            channels_with_wanted_final_cap++;
        if(channels_with_wanted_final_cap>=2)
            break;
    }
    if(channels_with_wanted_final_cap<2)
        return true;

    return false;
}

bool Config::excludesEdge(const NetworkSummary& network, const Edge& edge) const
{
  const Config& cfg(*this);

  if(cfg.minRoutingCap > edge.capacity)
      return true;
  //biggest of max_htlc_msat of both sides should also be greater than tested cpacity

  //zbf edges filtering : not filtered here, depends on the side. We can only prefilter the ones that are not zbf in both sides
  if(cfg.zbfPaths && !edge.side[0].isZbf && !edge.side[1].isZbf)
      return true;

  const Node& node0 = network.nodes[edge.side[0].node_rank];
  const Node& node1 = network.nodes[edge.side[1].node_rank];
  //Filter edges not on clearnet?
  bool reachable_on_clearnet = (cfg.clearnetEdges && node0.clearnet && node1.clearnet);
  bool reachable_on_tor = (cfg.torEdges && node0.tor && node1.tor);
  if(!(reachable_on_clearnet||reachable_on_tor))
      return true;

  return false;
}
