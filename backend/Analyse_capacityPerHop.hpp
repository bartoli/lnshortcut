#ifndef ANALYSE_CAPACITYPERHOP_HPP
#define ANALYSE_CAPACITYPERHOP_HPP

#include <vector>
#include <stdint.h>
#include <QMap>

using ReachTree = std::vector<std::vector<int32_t>>;
class Hopness;
class NetworkSummary;
class Config;


void capacity_per_hop(ReachTree& reach_tree, const NetworkSummary& networkRef, Hopness& result, int node0_rank, const Config& config, std::vector<int> testEdges=std::vector<int>());

/*
 * Analyse the capacity reachable for a given fee. Propose a channel, which, for a selected fee, brings the most liquidity reachable by a given fee in a given direction.
 *
 * We browse each channel, saving it's path while the total fee to reach it is bellow the set threshold.
 * the capacity to compare is the sum of capacities of the edges reached
 *
 * For directions, we simply take the fee from side 1 or 2 of the chanel, depending on which node we come from, and if we are analysing direction inwards or outwards
 * As fees are different in both directions, we have 2 separate graph searches for each direction (in two threads?)
 *
 * For selection of the best candidate, on the selected side, we re-evaluate the reached capacity with a simulated connection on each of the unconnected edges
 * Reached capacity will be compared by a basic number compatrison here, since we don't care about hops, just maximum fees
 *
 * Taking into account the initial search data, we can try to only browse the new edges that are reachable by the tested connection below the wanted capacity
 * If this is possible, then we can compare that capacity the channel brings nearer, instead of the total reachable capacity
 *
 * Ideally, it could be nice to ba able to select only a channel that has low traffic in one direction, and chose that specific connection that will bring the most for that chanel
 */

typedef enum
{
    INBOUND=0,
    OUTBOUND=1,
    BOTH=2,
    BOTH_AVG=3,
    BOTH_BALANCED=4,
    COUNT=5
} LiquidityDirection;

/*
 * @param max_fee_msat max   fee to calculate reach
 * @param basefee_msat  wanted fee (rate) for inbound connections from this channel
 * @param feerate_msat  wanted fee (rate) for outbound connections from this channel
 */

typedef struct
{
    const NetworkSummary& network;
    const Config& config;
    int node0_rank;
    uint64_t max_fee_sat;
    uint64_t test_amt_sat;
    int max_hops=5;
    LiquidityDirection testDirection;
}CFF_Params;

typedef struct CFF_Result
{
    //reached nodes during the route analysis. The value is the smallest cost to reche them
    std::vector<uint64_t>reached_nodes;
    //reched edges during route analysis. Here, we only stock whether the ede has been reached or not, not the cost, beacuase
    //-reached cost is only for crosing te edge in one direction. reach_cost can only be compared for a specific node
    //-so we will still re-cross an edge even if it is for a hogher cost, because we might
    std::vector<bool> reached_edges;

    typedef enum{
      REFERENCE=0,       //Result with existing channels
      MOST_NEW_EDGES=1,  //Result for node that brings the most new reacable edges
      MOST_NEW_NODES=2,  //result for the node that brings the most new reachable nodes
      BEST_MEDIAN_COST=3,//result for the node that brinds he smallest median fee for original reachable nodes
      COUNT=4
    }RankingCategory;
    size_t best_candidate_rank[RankingCategory::COUNT];
    int new_reached_edges[RankingCategory::COUNT];
    int new_reached_nodes[RankingCategory::COUNT];
    int median_cost_for_original_reach[RankingCategory::COUNT];

    CFF_Result(const CFF_Params& params);
}CFF_Result;

void analyze_candidates(const CFF_Params& params, CFF_Result& result);
void capacity_for_fee(const CFF_Params& params,
                      CFF_Result& result);

#endif // ANALYSE_CAPACITYPERHOP_HPP
