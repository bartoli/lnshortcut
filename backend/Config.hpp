#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <QScopedPointer>
#include <QSet>
#include <QString>

class Node;
class Edge;

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
    QSet<QString> ignored_endpoint_nodes;
    //Ignore existing channels from node0 to those nodes (to simulate bes channel to close for example)
    QSet<QString> ignore_connected_nodes;
    //Automatically choose, for the wanted channel capacity, the channels to close that make the best capacity reallocation
    quint64 cap_to_reclaim = 0;

    //Hop level for candidates;
    int candidates_depth;

    qint64 minCap;
    //ZeroBaseFee options
    //Connect only to nodes that respect zbf
    bool zbfEndpoints = true;
    //Analyze ony zbf paths
    bool zbfPaths = true;
    bool clearnetNodes = true;
    bool torNodes=true;


    bool excludesNode(const Node& node) const;
    bool excludesEdge(const Edge& node) const;

};

#endif // CONFIG_HPP
