
#include <Analyse_capacityPerHop.hpp>
#include <Config.hpp>
#include <Hopness.hpp>
#include <NetworkSummary.hpp>

#include <QDebug>
#include <set>

#include <queue>

//Compute, for each level of hops, the reached capacity
//testEdges: node ranks to add simulated edge to
void capacity_per_hop(ReachTree& reach_tree, const NetworkSummary& networkRef, Hopness& result, int node0_rank, const Config& config, std::vector<int> testEdges)
{
    //list of reached node ranks for each level of hops from origin
    //std::vector<std::vector<qint32>> reach_tree(20);
    reach_tree.clear();
    reach_tree.resize(20);

    const NetworkSummary* _network = &networkRef;
    NetworkSummary netCpy;
    if(!testEdges.empty())
    {

        netCpy = networkRef;
        for(size_t i=0;i<testEdges.size();++i)
        {
          //     qWarning()<<QString::number(testEdges[i]);
          Edge e;
          e.side[0].node_rank = node0_rank;
          e.side[1].node_rank = testEdges[i];
          e.capacity = config.minRoutingCap;
          e.capacity_counted = -1;
          netCpy.edges.push_back(e);
          int edge_rank = netCpy.edges.size()-1;
          netCpy.nodes[node0_rank].edges.push_back(edge_rank);
          netCpy.nodes[testEdges[i]].edges.push_back(edge_rank);
        }
        _network = &netCpy;
    }

    //patch network with tesed edges


    //Prepare result
    //result.edge_directions.clear();
    /*int node_count = network.nodes.size();
    result.edge_directions.resize(node_count);
    for(int i=0; i<node_count; ++i)
    {
        result.edge_directions[i].resize(network.nodes[i].edges.size(),EdgeDirection::UNKNOWN);
    }*/
    result.edge_capacity_counted.resize(_network->edges.size(), -1);

    //for each node, minimum search depth (hop count-1) to reach it
    QVector<int> reached_nodes(_network->nodes.size(), -1);
    reached_nodes[node0_rank] = 0;//1-based depth where the node was reached (number of hops, 0 for root, -1 for unreached)
    //current depth of hops (0 for first hop)
    int search_depth = 0;
    //min edge size to use

    //Set the direction of a edge from one node's side
    /*auto set_edge_io = [](const Node&node, NodeEdgesDirection& directions, int edge, EdgeDirection dir)
    {
        for (size_t e=0;e<node.edges.size();++e)
        {
            if(node.edges[e] == edge)
            {
              if(directions[e] == EdgeDirection::UNKNOWN || directions[e] == EdgeDirection::FURTHER)
              {
                  directions[e] = dir;
                  return;
              }
            }
        }
    };*/

    //Look at all the edges of a node (of previous hop level).
    //the edges to not reached yet nodes will be in the next hop level
    auto analyze_node_edges = [&](qint32& node)
    {
        //search depth : number of hops from orig for the current nodes

        const Node& nodeObj = _network->nodes[node];
        QSet<int> nodes_seen; //nodes already seen by the node
        for (int i=0, cnt=nodeObj.edges.size();i<cnt;++i)
        {
          int edge_rank = nodeObj.edges[i];
          const Edge& edge = _network->edges[edge_rank];
          //if(nodeObj.pubKey == "02029a5ea890afa1aa8a201ae66fabaaaa50bc5735bbc88239d9f05d241a328c99")
          //    printf("%d\n", edge.capacity);
          if(edge.capacity<config.minRoutingCap)
              continue;
          if(config.zbfPaths && !(edge.side[0].isZbf && edge.side[1].isZbf))
              continue;
          int other_node_rank = edge.side[0].node_rank==node? edge.side[1].node_rank : edge.side[0].node_rank;

          //const Node& other_node = network.nodes[other_node_rank];
          if(result.edge_capacity_counted[edge_rank] <0)
          {
            result.capacities[search_depth] += std::min<long long>(config.minRoutingCap, edge.capacity);
            result.edge_capacity_counted[edge_rank] = search_depth;
          }

          int seen_depth = reached_nodes[other_node_rank];
          if(seen_depth<0)
          {
              //TODO? should we count the min_capacity instead of the chanels' capacity??
              /*set_edge_io(nodeObj, result.edge_directions[node], edge_rank, EdgeDirection::FURTHER);
              set_edge_io(other_node, result.edge_directions[other_node_rank], edge_rank, EdgeDirection::NEARER);*/

              reached_nodes[other_node_rank] = search_depth+1;
              reach_tree[search_depth].push_back(other_node_rank);
              nodes_seen.insert(other_node_rank);
          }
          /*else
          {
            if(seen_depth==search_depth){
              set_edge_io(nodeObj, result.edge_directions[node], edge_rank, EdgeDirection::SAME);
              set_edge_io(other_node, result.edge_directions[other_node_rank], edge_rank, EdgeDirection::SAME);
            }
            else if(seen_depth<search_depth)
            {
                set_edge_io(nodeObj, result.edge_directions[node], edge_rank, EdgeDirection::NEARER);
                set_edge_io(other_node, result.edge_directions[other_node_rank], edge_rank, EdgeDirection::FURTHER);
            }
          }*/
        }
    };//analyze_node()

    const Node node0 = _network->nodes[node0_rank];
    //std::fill(result.edge_directions[node0_rank].begin(), result.edge_directions[node0_rank].end(), EdgeDirection::FURTHER);
    result.capacities.append(0);
    analyze_node_edges(node0_rank);
    //qWarning()<<QString("Reached capacity : %1").arg(result.capacities[search_depth]/100000000.0);
    //int total_reached_nodes=0;
    while(!reach_tree[search_depth].empty())
    {
      result.capacities.append(0);
      int reached_depth = search_depth; //depth of the nodes to analyze edees of
      ++search_depth; //newly reached depth to put analyse results to
      int reached_count = reach_tree[reached_depth].size();
      //total_reached_nodes += reached_count;
      /*qWarning()<<QString("Depth %1 : %2 nodes reached")
                  .arg(reached_depth)
                  .arg(reached_count);*/


      //qint64 reached_capacity=0;
      QSet<int> edges_on_same_level;
      QSet<int> edges_from_previous_level;
      for(int i=0; i<reached_count; ++i)
      {
          int reached_node_rank = reach_tree[reached_depth][i];
          analyze_node_edges(reached_node_rank);
          /*const Node& reached_node = network.nodes[reached_node_rank];
          for(size_t j=0; j< reached_node.edges.size();++j)
          {
              const EdgeDirection& dir = result.edge_directions[reached_node_rank][j];
              int edge_rank = reached_node.edges[j];
              if(dir == EdgeDirection::SAME)
                  edges_on_same_level.insert(edge_rank);
              else if(dir == EdgeDirection::NEARER)
                  edges_from_previous_level.insert(edge_rank);
          }*/
      }
      /*for(const int& rank : edges_on_same_level)
      {
          if(network.edges[rank].capacity_counted <0)
          {
            reached_capacity += network.edges[rank].capacity;
            result.edge_capacity_counted[rank] = reached_depth;
          }
          else
            puts("Bug");
      }
      for(const int& rank : edges_from_previous_level)
      {
          if(network.edges[rank].capacity_counted <0)
          {
            reached_capacity += network.edges[rank].capacity;
            result.edge_capacity_counted[rank] = reached_depth;
          }
          else
            puts("Bug");
      }*/

      //qWarning()<<QString("Reached capacity : %1").arg(result.capacities[search_depth]/100000000.0);
    }
    //qWarning()<<QString("Reached %1 nodes (max %2 hops)").arg(total_reached_nodes).arg(search_depth);
};//capacity_for_hops()

//std::function<void(int,int,uint64_t,uint64_t)> browse_edge;

CFF_Result::CFF_Result(const CFF_Params& params)
{
    reached_nodes.resize(params.network.nodes.size(),ULONG_LONG_MAX);
    reached_edges.resize(params.network.edges.size(),ULONG_LONG_MAX);
}

void browse_edge(const CFF_Params& params, CFF_Result& result,
        int browsed_edge_rank, int nearer_node_rank,
        uint64_t current_cost_msat);
void browse_node(const CFF_Params& params, CFF_Result&,
        int node_rank, uint64_t current_cost_msat, int origin_edge_rank);

void browse_node(const CFF_Params& params, CFF_Result& result,
        int node_rank, uint64_t current_cost_msat, int origin_edge_rank)
{
    //called only at first reach, or new reach with a lower cost. update new reach cost
    result.reached_nodes[node_rank] = current_cost_msat;
    const Node& reached_node = params.network.nodes[node_rank];
    //if(current_cost_msat<(max_fee_sat*1000))//useless iff, otherwise, the edge leading to it wouldn't have been crossed?
    {
        for(int ie=0, cnt=reached_node.edges.size();ie<cnt;++ie)
        {
            int edge_rank = reached_node.edges[ie];
            //Do not go back and forth on the same edge
            if(edge_rank == origin_edge_rank)
                continue;

            if(params.config.zbfPaths)
            {
              const Edge& edge = params.network.edges[edge_rank];
              int other_side = edge.side[0].node_rank == node_rank? 0 : 1;
              if(!edge.side[other_side].isZbf)
                  continue;
            }


            //assume a channel is on average with 50% cap on each side?
            //to be replaced by max htlc_test on the current direction
            /*const Edge& edge = network.edges[edge_rank];
            if(edge.capacity<width_sat*2ULL)
                continue;*/

            browse_edge(params, result, edge_rank, node_rank, current_cost_msat);
        }
    }
};

//browse a new node and beyond (recursive until a max_cost is reached)
void browse_edge(const CFF_Params& params, CFF_Result& result,
        int browsed_edge_rank, int nearer_node_rank,
        uint64_t current_cost_msat)
{
  //qWarning() << "Browsing edge "<<browsed_edge_rank;
  const Edge& edge = params.network.edges[browsed_edge_rank];
  const int nearer_side = edge.side[0].node_rank == nearer_node_rank? 0:1;
  const int further_side = 1-nearer_side;

  /*
   * Side whose fee to account
   * If LiquidityDirection = Outbound, we calculate fee for transacton from origin_node to the new one.
   * the fee paid is the one on the destination side of an edge, so on the destination side.
   */
   const int side_for_fee = (params.testDirection == LiquidityDirection::OUTBOUND? further_side : nearer_side);
   const Edge::Side& fee_side = edge.side[side_for_fee];
   if(fee_side.disabled)
       return;
   if(fee_side.max_htlc_msat < params.test_amt_sat*1000ULL)
       return;

   const int64_t edge_cost_msat = fee_side.base_fee_msat + (fee_side.feerate_msat*params.test_amt_sat)/1000;
   if(current_cost_msat+edge_cost_msat > (params.max_fee_sat*1000))
   {
       //qWarning()<<"Max cost reached";
       //Max cost reached. we can not cross this edge. Return
       return;
   }
   current_cost_msat += edge_cost_msat;

   //edge is crossable from this path.
   //Mark the edge as as reached, and add current cost as reach cost if smaller than previous
   result.reached_edges[browsed_edge_rank] = std::min<uint64_t>(current_cost_msat, result.reached_edges[browsed_edge_rank]);
   //We do not stop here if edge was already reached with a lower cost.
   //We might have encountered a loop and ned to browse the node from the other direction
   //it will stop anyways if it makes us reach a node we already reached for a lower cost

   //If dest node was not already in browsed nodes, we will need to browse its edges next
   const int further_node_rank = edge.side[further_side].node_rank;
   //We do not rebrowse the node if it was reached from a different path for the same cost,
   //since it would just reach the same edges
   //then lead to an infinite loop
   if(current_cost_msat >= result.reached_nodes[further_node_rank] )
   {
       //qWarning()<<"Reached node already browsed";
       return;
   }
   //Look at it's edges if we can reach more
   browse_node(params, result, further_node_rank, current_cost_msat, browsed_edge_rank);
};

//Compute reached nodes count, and median cost to reach them
int median_node_reach_cost(const CFF_Params &params,
        const std::vector<uint64_t> &reached_nodes,
        int &reached_nodes_count)
  {
    int median_cost_msat = -1;
        reached_nodes_count = 0;
    QVector<uint64_t> reach_costs;
    reach_costs.reserve(reached_nodes.size());
    for (uint64_t in=0, ncnt = reached_nodes.size(); in<ncnt; ++in)
    {
        if(in == params.node0_rank)
            continue;
        auto cost = reached_nodes[in];
        if(cost == ULONG_LONG_MAX)
            continue;
        reach_costs.push_back(cost);
        ++reached_nodes_count;
    }
    if(reached_nodes_count>0)
    {
        std::sort(reach_costs.begin(), reach_costs.end());
        int median_node = reached_nodes_count/2;
        median_cost_msat = reach_costs[median_node];
    }
    return median_cost_msat;
};

void analyze_candidates(const CFF_Params& params, CFF_Result& result)
{
  //Now evaluate the extra connecton for each candidates
  //Build list of nodes to avoid evaluating
  QSet<int> already_connected_ranks;
  already_connected_ranks.insert(params.node0_rank);
  const Node& node0 = params.network.nodes[params.node0_rank];
  for (auto edge_rank : node0.edges)
  {
     const Edge& edge = params.network.edges[edge_rank];
     int other_side = edge.side[0].node_rank==params.node0_rank? 1:0;
      already_connected_ranks.insert(edge.side[other_side].node_rank);
  }

  /*auto new_reached_nodes = reached_nodes;
  auto new_reached_edges = reached_edges;*/
  int best_candidate_for_edges = -1;
  int best_candidate_for_nodes = -1;
  int best_reached_nodes=0;
  int best_reached_nodes2=0;
  int best_reached_edges=0;
  int best_reached_edges2=0;
  int best_candidate_for_cost = params.node0_rank;
  int best_median_cost=result.median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];

  for(size_t reached_node_rank=0; reached_node_rank < result.reached_nodes.size(); ++reached_node_rank)
  {
      /*if(reached_nodes[reached_node_rank]==ULONG_LONG_MAX)
          return;*/
      if(already_connected_ranks.contains(reached_node_rank)) //TODO? replace by a sentinel value somewhere?
          continue;
      if(params.network.ignored_endpoints.contains(reached_node_rank)/*config.ignored_endpoint_nodes.contains(candidate_node.pubKey)*/)
          continue;
      const Node& candidate_node = params.network.nodes[reached_node_rank];
      if(params.config.excludesNodeAsEndPoint(candidate_node, params.network))
          continue;
      if(candidate_node.median_fee_ppm>1000)
          continue;
      CFF_Result new_result(result);
      /*new_reached_nodes = reached_nodes;
      new_reached_edges = reached_edges;*/
      /*
       * Estimate new reach for a candidate, assuming channel fees will be the nodes'median fees
       * For OUTBOUND test, first channel fee is the median fee from the original node
       * For INBVOUND fee, first channel fee is the median fee of the tested node
       */
      int node_base_fee = params.testDirection == LiquidityDirection::OUTBOUND? node0.median_fee_ppm : candidate_node.median_fee_ppm;
      browse_node(params, new_result,
                  reached_node_rank, node_base_fee, -1);
      int new_reached_edges_count = params.network.edges.size() - std::count(new_result.reached_edges.begin(), new_result.reached_edges.end(), ULONG_LONG_MAX);
      int new_reached_nodes_count = 0;//params.network.nodes.size() - std::count(new_result.reached_nodes.begin(), new_result.reached_nodes.end(), ULONG_LONG_MAX);
      int new_median_node_reach_cost = median_node_reach_cost(params, new_result.reached_nodes, new_reached_nodes_count);
      if(new_median_node_reach_cost < best_median_cost)
      {
          best_candidate_for_cost = reached_node_rank;
          best_median_cost = new_median_node_reach_cost;
      }
      /*qWarning() << network.nodes[node_rank].pubKey;
      qWarning() << new_reached_edges_count - reached_edges_count << "edges reached";
      qWarning() << new_reached_nodes_count - reached_nodes_count << "nodes reached";*/
      if(best_reached_edges<new_reached_edges_count ||
              (best_reached_edges == new_reached_edges_count && best_reached_nodes<new_reached_nodes_count))
      {
          best_candidate_for_edges = reached_node_rank;
          best_reached_edges = new_reached_edges_count;
          best_reached_nodes = new_reached_nodes_count;
      }
      if(best_reached_nodes2<new_reached_nodes_count ||
              (best_reached_nodes2 == new_reached_nodes_count && best_reached_edges2<new_reached_edges_count))
      {
          best_candidate_for_nodes = reached_node_rank;
          best_reached_edges2 = new_reached_edges_count;
          best_reached_nodes2 = new_reached_nodes_count;
      }
  } // end of candidates loop

  result.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_EDGES] = best_candidate_for_edges;
  result.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_EDGES] = best_reached_edges;
  result.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_EDGES] = best_reached_nodes;
  result.best_candidate_rank[CFF_Result::RankingCategory::MOST_NEW_NODES] = best_candidate_for_nodes;
  result.new_reached_edges[CFF_Result::RankingCategory::MOST_NEW_NODES] = best_reached_edges2;
  result.new_reached_nodes[CFF_Result::RankingCategory::MOST_NEW_NODES] = best_reached_nodes2;
  result.best_candidate_rank[CFF_Result::RankingCategory::BEST_MEDIAN_COST] = best_candidate_for_cost;
  result.median_cost_for_original_reach[CFF_Result::RankingCategory::BEST_MEDIAN_COST] = best_median_cost;

  /*qWarning() << network.nodes[best_candidate_for_edges].pubKey;
  qWarning() << best_reached_edges << "edges reached";*/
}; //analyze_candidates()

void capacity_for_fee(const CFF_Params& params,
                      CFF_Result& result)
{
  /*TODO:
    Assume that a loop always only adds cost. So keep the list of reached edges for a recursion branch, and do not got through nodes already reached on that branch


  */

  /*
  //Work Data shared with the recursive routine
  // already reached nodes/edges for this browse. the value is the smallest cost found to reach it
  std::vector<uint64_t> reached_nodes(params.network.nodes.size(), ULONG_LONG_MAX);
  // already reached_edges. the value is the smallest cost found to reach it
  std::vector<uint64_t> reached_edges(params.network.edges.size(), ULONG_LONG_MAX);*/

  //Called for each edge of a node.
  //It will call itself while a path contains not yet used edges and total fee from origin to las node is not the max one

  //aNALYSIS STARTING POINT
  browse_node(params, result, params.node0_rank, 0, -1);

  int reached_nodes_count=0;  
  int median_cost_msat = median_node_reach_cost(params, result.reached_nodes, reached_nodes_count);

  //qWarning() << reached_sats/100000000.<<"BTC reached";
  int reached_edges_count = params.network.edges.size() - std::count(result.reached_edges.begin(), result.reached_edges.end(), ULONG_LONG_MAX);
  //int reached_nodes_count = network.nodes.size() - std::count(reached_nodes.begin(), reached_nodes.end(), ULONG_LONG_MAX);
  qWarning() << reached_edges_count << "edges reached";
  qWarning() << reached_nodes_count << "nodes reached";
  qWarning() << median_cost_msat/1000.0 << "of median node reach cost";

  result.best_candidate_rank[CFF_Result::RankingCategory::REFERENCE] = params.node0_rank;
  result.new_reached_edges[CFF_Result::RankingCategory::REFERENCE] = reached_edges_count;
  result.new_reached_nodes[CFF_Result::RankingCategory::REFERENCE] = reached_nodes_count;
  result.median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE] = median_cost_msat;
}
