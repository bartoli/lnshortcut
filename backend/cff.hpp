#ifndef CFF_HPP
#define CFF_HPP

#include "NetworkSummary.hpp"


//Code for "Capacity For Fee' evaluation

typedef enum
{
    INBOUND=0,
    OUTBOUND=1,
    DIRECTION_COUNT=2,

    BOTH=2,
    BOTH_AVG=3,
    BOTH_BALANCED=4,
    WORST_DIRECTION=5,
    RANKING_DIRECTION_COUNT=6

} LiquidityDirection;

/*
 * @param max_fee_msat max   fee to calculate reach
 * @param basefee_msat  wanted fee (rate) for inbound connections from this channel
 * @param feerate_msat  wanted fee (rate) for outbound connections from this channel
 */

typedef struct CFF_Params
{
    const NetworkSummary& network;
    const Config& config;
    int node0_rank;
    uint64_t max_fee_sat;
    uint64_t test_amt_sat;
    int max_hops=5;
    LiquidityDirection testDirection;
}CFF_Params;

typedef struct NodeReach
{
  //WORK DATA
  //reached nodes during the route analysis. The value is the smallest cost to reche them
  std::vector<uint64_t>nodes;
  //reched edges during route analysis. Here, we only stock whether the ede has been reached or not, not the cost, beacuase
  //-reached cost is only for crosing te edge in one direction. reach_cost can only be compared for a specific node
  //-so we will still re-cross an edge even if it is for a hogher cost, because we might
  std::vector<bool> edges;

  //RESULT DATA
  int reached_nodes;
  int reached_edges;
  int cheaperNodes = 0;
  int median_cost_msat;
  void preprocess(const CFF_Params&);
  void preprocessDelta(const CFF_Params&, const NodeReach& refReach);
  //void preprocessDelta(const CFF_NodeReach&);

}CFF_NodeReach;

typedef enum RankingCategory
{
  //REFERENCE=0,          //Result with existing channels
  NEW_EDGES=0,     //Result for node that brings the most new reacable edges
  NEW_NODES=1,     //result for the node that brings the most new reachable nodes
  MEDIAN_COST=2,   //result for the node that brings the smallest median fee (for originaly reachable nodes)
  CHEAPER_NODES=3, //same as MOST_NEW_NODES, but includes currently reached nodes that now have alower reach cost
  //MOST_CHEAPED_EDGES=4, //same as MOST_NEW_EDGES, but includes currently reached edges that now have alower reach cost
  COUNT=4
}RankingCategory;

typedef struct CFF_Metric
{
    int rank=-1;
    int newNodes = 0;
    int newNodes2 = 0;
    int cheaperNodes = 0;
    int cheaperNodes2 = 0;
    int newEdges = 0;
    int newEdges2 = 0;
    int medianCost = INT_MAX;

}CFF_Metric;


typedef struct CFF_Result
{
    //Work data during analysis
    CFF_NodeReach refReach[LiquidityDirection::DIRECTION_COUNT];
    //CFF_NodeReach newReach[LiquidityDirection::DIRECTION_COUNT];
    //results
    CFF_Metric metrics[LiquidityDirection::RANKING_DIRECTION_COUNT][RankingCategory::COUNT];

    void initializeWorkData(const CFF_Params&);
    void initializeResults(const CFF_Params&);
}CFF_Result;

/*void analyze_ref_reach(const CFF_Params& params, CFF_Result& result);
void analyze_candidates(const CFF_Params& params, CFF_Result& result);*/
void run_CFF(const CFF_Params&, CFF_Result&);


#endif // CFF_HPP
