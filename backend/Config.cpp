#include <Config.hpp>
#include <NetworkSummary.hpp>

Config::Config()
{

    ignored_endpoint_nodes = QSet<QString>(
    {
        //satoshibox, channel disabled half of the time, could not do initial rebalance and no traffic for more than a week
        "03fa3be3b4528c1d588e60060d7f10e37c38f5aa02f867632a5c7b816cf5afa775",
        //NateNate60s-node, tcp connection refused when adding as peer
        "033a7fca1cb089b33ac232dd73c2c32aeacffe16319b72bec16208f3a9002e6d17",
        //Satoshi Radio Lightning Node 2, Not really accepting clearnet connection
        "03dc8f04a944b498caca1bd666d09184d5a180a50818316ea447508a5940800b40",
        //PaymentHub, connected to us for test at 4 Msat, but disconnected. Might disconnect again?
        "029ed7e456a53568e12f013173dc5be2a92c86302e15038d150362933b754e5d21",
        //BitSoapBox, became inactive, then did a remote force close
        "021607cfce19a4c5e7e6e738663dfafbbbac262e4ff76c2c9b30dbeefc35c00643",

        //Future interesting channels, but thhat require to big chan sise
        //SunnySarah, min=0.04
        "03423790614f023e3c0cdaa654a3578e919947e4c3a14bf5044e7c787ebd11af1a",
        //Garlic, min = 0.04
        "033b63e4a9931dc151037acbce12f4f8968c86f5655cf102bbfa85a26bd4adc6d9",
        //bfx-lnd0 min = 0.04BTC
        "033d8656219478701227199cbd6f670335c8d408a92ae88b962c49d4dc0e83e025",
        //bfx-lnd1, idem
        "03cde60a6323f7122d5178255766e38114b4722ede08f7c9e0c5df9b912cc201d6",
        //WalletofSatoshi, 0.05BTC
        "035e4ff418fc8b5554c5d9eea66396c227bd429a3251c8cbc711002ba215bfc226",
        //Nordicrails, min= 0.05
        "033d9e73a183c9714545f292875fb90c4372bddc9c2cc302b265d15e7969a5ed60",
        //LnCapital, min=0.06
        "0326e692c455dd554c709bbb470b0ca7e0bb04152f777d1445fd0bf3709a2833a3",
        //Breez, 10M
        "031015a7839468a3c266d662d5bb21ea4cea24226936e2864a7ca4f2c3939836e0",
        //River Financial 1 : min=0.1BTC
        "03037dc08e9ac63b82581f79b662a4d0ceca8a8ca162b1af3551595b8f2d97b70a",
        //cyberhub, min=0.1
        "03dc686001f9b1ff700dfb8917df70268e1919433a535e1fb0767c19223509ab57",
        //yalls.org TOR, 0.16777215BTC
        "03d06758583bb5154774a6eb221b1276c9e82d65bbaceca806d90e20c108f4b1c7",
        //yalls.org clearnet, idem
                    "0288be11d147e1525f7f234f304b094d6627d2c70f3313d7ba3696887b261c4447",
        // ⚡G-Spot-21_69_420⚡, 0.0696942 Btc
        "03c5528c628681aa17ab9e117aa3ee6f06c750dfb17df758ecabcd68f1567ad8c1"


    });

    lns_pubkey = "02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683";

    //0-based hop count for list of candidates
    candidates_depth = 1;
}

bool Config::excludesNode(const Node& node) const
{
  const Config& cfg(*this);

  bool reachable_on_clearnet = node.clearnet && cfg.clearnetNodes;
  bool reachable_on_tor = node.tor && cfg.torNodes;
  if (!(reachable_on_clearnet || reachable_on_tor))
          return true;

  return false;
};

bool Config::excludesEdge(const Edge& edge) const
{
  const Config& cfg(*this);

  if(cfg.minCap > edge.capacity)
      return true;
  //biggest of max_htlc_msat of both sides should also be greater than tested cpacity

  return false;
}
