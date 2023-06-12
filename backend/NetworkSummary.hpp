#ifndef NETWORKSUMMARY_HPP
#define NETWORKSUMMARY_HPP

#include <QString>
#include <qglobal.h>
#include <limits.h>
#include <QSet>
#include <QMap>
 class Config;

class Node
{
public:
    //node public key
    QString pubKey;
    //edges of the node
    std::vector<qint32> edges;
    //direction of each edge rleative to analysez node (does the edge go nearer or further from/tp the analyzed node?)
    //std::vector<EdgeDirection> edges_direction; //is each edge bringing nearer, further, or at same dist from us?
    //does the node have a clearnet address
    bool clearnet=false;
    //does the node have a tor address
    bool tor=false;
    //does te node have 0 base fee on his side of all channels
    bool isZbf=true;
    //smallest channel size of the node
    uint32_t minChanSize = INT_MAX;
    //node alias
    QString alias;
    //is the node allowed as endoiint? Yes unless otherwise specified
    bool validEndpoint = true;
    // median fee on the node's side
    int median_fee_ppm=0;
};
class Edge
{
public:
  //Node ranks. First is the smallest, not the initiator
  uint32_t capacity;
  int capacity_counted=-1;

  typedef struct
  {
    int node_rank;
    int base_fee_msat=0;
    int feerate_msat = 0;
    uint64_t max_htlc_msat = 0;
    bool disabled = false;
    bool isZbf = true;
  }Side;
  Side side[2];
};


/*
 * Sum of prefetched data from a network graph at a given block height
 */

class NetworkSummary
{
public:
    NetworkSummary() = default;

    //info about the analysis itself
    //onchain height when the network graph was dumped
    int block_height;
    //analysis start timestamp (for proper exclusion of nodes with old updates)
    int timestamp;

    //raw Network data
    //Info for all edges. Filled at the start. then the rank in this array is used as index for faster edge dereferencing
    std::vector<Edge> edges;
    //same for nodes
    std::vector<Node> nodes;
    //node rank from pubkey
    QMap<QString, qint32> node_index;

    //Stats about the network as a whole
    uint64_t total_capacity;

    //Stats about LNS node
    int lns_noderank;

    //Pre-fetched analysis data
    int zbfNodes = 0;
    int zbfEdges = 0;

    // ignored endpoints (for current config), indexed by local node rank
    QSet<int> ignored_endpoints;

    NetworkSummary filter(const Config&, int node0_rank) const;
    void updateIgnoredEndpoints(const Config&);
};

#endif // NETWORKSUMMARY_HPP
