#include "Config.hpp"

QScopedPointer<Config> Config::_instance;

Config* Config::getInstance()
{
    if(_instance.isNull())
    {
        _instance.reset(new Config());
        _instance->_initConfig();
    }
    return _instance.data();
}

void Config::_initConfig()
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
        //bfx-lnd0 min = 0.04BTC
        "033d8656219478701227199cbd6f670335c8d408a92ae88b962c49d4dc0e83e025",
        //bfx-lnd1, idem
        "03cde60a6323f7122d5178255766e38114b4722ede08f7c9e0c5df9b912cc201d6",
        //WalletofSatoshi, 0.05BTC
        "035e4ff418fc8b5554c5d9eea66396c227bd429a3251c8cbc711002ba215bfc226",
        //Breez, 10M
        "031015a7839468a3c266d662d5bb21ea4cea24226936e2864a7ca4f2c3939836e0",
        //River Financial 1 : min=0.1BTC
        "03037dc08e9ac63b82581f79b662a4d0ceca8a8ca162b1af3551595b8f2d97b70a"

    });

    lns_pubkey = "02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683";

    //0-based hop count for list of candidates
    candidates_depth = 1;

    //ZeroBaseFee options
    //Connect only to nodes that respect zbf
    bool zbfEndpoints = false;
    //Analyze ony zbf paths
    bool zbfPaths = false;

}
