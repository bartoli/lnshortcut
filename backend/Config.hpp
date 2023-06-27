#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <QMultiMap>
#include <QScopedPointer>
#include <QSet>
#include <QString>

class Node;
class Edge;
class NetworkSummary;

/*
 * Global config
 */
class Config
{
public:
    Config();
    //static Config* getInstance();

    //Analysis config params.
    //TO BE MOVED TO A SEPARATE OBJECT
    QString lns_pubkey;
    QSet<QString> ignored_routing_nodes;
    //Nodes that shuld not be recommended as
    //QSet<QString> ignored_endpoint_nodes;
    //Ignore existing channels from node0 to those nodes (to simulate bes channel to close for example)
    QSet<QString> ignore_connected_nodes;
    //Database of known minimum channel size, ordered by minimum channel size
    QMultiMap<uint32_t, QString> min_chan_size_db;
    //Automatically choose, for the wanted channel capacity, the channels to close that make the best capacity reallocation
    quint64 cap_to_reclaim = 0;

    //Hop level for candidates;
    int candidates_depth;

    //Capacity filtering
    qint64 minRoutingCap; //minimum capacity a channel must be able to route to be browser
    qint64 minEndpointCap; //a potential candidate must have at least 2 channels of this min capacity to be considered as an intersint grouting peer
    //TODO : for endpoouints, the two channel of min cap must be also two channels that have each the min routing capacity in both direction

    //ZeroBaseFee options
    //Connect only to nodes that respect zbf
    bool zbfEndpoints = true;
    //Analyze ony zbf paths
    bool zbfPaths = true;
    //clearnet/tor endpoints?
    bool clearnetNodes = true;
    bool torNodes=true;
    //analyse only clearnet/tor routes? (edges where both sides are on tor/clearnet)
    bool torEdges = true;
    bool clearnetEdges = true;


    bool excludesNodeForRouting(const Node& node) const;
    bool excludesNodeAsEndPoint(const Node& node, const NetworkSummary& network) const;
    bool excludesEdge(const NetworkSummary& network, const Edge& node) const;

};

#endif // CONFIG_HPP
