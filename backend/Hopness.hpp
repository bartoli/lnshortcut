#ifndef HOPNESS_HPP
#define HOPNESS_HPP

#include <vector>
#include <qglobal.h>
#include <EdgeDirection.hpp>
#include <QList>

typedef std::vector<EdgeDirection> NodeEdgesDirection;


//Class for results of analysis  of how many nodes are reached through each new hop from a node
class Hopness
{
public:
    Hopness() = default;

    //Params for this hopness result
    int node0;
    //int minChanSize;

    //Temp work params
    //hop level where a node was reached (1 value per node in network, negative if not reached yet)
    //std::vector<int> reached_nodes;

    //temp work data
    //List of reached nodes
    //std::vector<int> nodes_reached;
    //std::vector<qint64> cap_reached;

    //cached values for other analyses
    //Direction of each edge of each node (nearer of further from original node
    //std::vector<NodeEdgesDirection> edge_directions;
    //depth rank where edge capacity was counted
    std::vector<int> edge_capacity_counted;
    //capacity reached at each new hop level
    QList<qint64> capacities;
};

#endif // HOPNESS_HPP
