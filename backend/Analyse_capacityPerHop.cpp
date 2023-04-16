
#include <Analyse_capacityPerHop.hpp>
#include <Config.hpp>
#include <Hopness.hpp>
#include <NetworkSummary.hpp>

#include <QDebug>
#include <set>

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
          e.capacity = config.minCap;
          e.capacity_counted = -1;
          e.isZbf = true;
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
          if(edge.capacity<config.minCap)
              continue;
          if(config.zbfPaths && !edge.isZbf)
              continue;
          int other_node_rank = edge.side[0].node_rank==node? edge.side[1].node_rank : edge.side[0].node_rank;

          //const Node& other_node = network.nodes[other_node_rank];
          if(result.edge_capacity_counted[edge_rank] <0)
          {
            result.capacities[search_depth] += std::min<long long>(config.minCap, edge.capacity);
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
    int total_reached_nodes=0;
    while(!reach_tree[search_depth].empty())
    {
      result.capacities.append(0);
      int reached_depth = search_depth; //depth of the nodes to analyze edees of
      ++search_depth; //newly reached depth to put analyse results to
      int reached_count = reach_tree[reached_depth].size();
      total_reached_nodes += reached_count;
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


void capacity_for_fee(const NetworkSummary& network, const Config& config,
                      int node0_rank, uint64_t max_fee_sat, uint64_t test_amt_sat,
                      const LiquidityDirection& testDirection)
{
  /*TODO:
    Assume that a loop always only adds cost. So keep the list of reached edges for a recursion branch, and do not got through nodes already reached on that branch


  */

  //Work Data shared with the recursive routine
  // already reached nodes/edges for this browse
  std::vector<uint64_t> reached_nodes(network.nodes.size(), ULONG_LONG_MAX);
  // already reached_edges. the value is the max channel capacity that could be used (depending on width of previous channels to it)
  std::vector<uint64_t> reached_edges(network.edges.size(), ULONG_LONG_MAX);

  //Called for each edge of a node.
  //It will call itself while a path contains not yet used edges and total fee from origin to las node is not the max one

  std::function<void(int,int,uint64_t,uint64_t)> browse_edge;
  auto browse_node = [&](int node_rank, uint64_t current_cost_msat, uint64_t width_sat, int origin_edge_rank)
  {
      reached_nodes[node_rank] = current_cost_msat;
      const Node& reached_node = network.nodes[node_rank];
      //if(current_cost_msat<(max_fee_sat*1000))//useless iff, otherwise, the edge leading to it wouldn't have been crossed?
      {
          for(int ie=0, cnt=reached_node.edges.size();ie<cnt;++ie)
          {
              int edge_rank = reached_node.edges[ie];
              //Do not go back and forth on the same edge
              if(edge_rank == origin_edge_rank)
                  continue;

              const Edge& edge = network.edges[edge_rank];
              if(edge.capacity<width_sat*2ULL)//assume a channel is on average with 50% cap on each side?
                  continue;
              browse_edge(edge_rank, node_rank, current_cost_msat, width_sat);
          }
      }
  };

  //browse a new node and beyond (recursive until a max_cost is reached
  browse_edge = [&](int browsed_edge_rank, int origin_node_rank, uint64_t current_cost_msat, uint64_t width_sat)
  {
    //qWarning() << "Browsing edge "<<browsed_edge_rank;
    const Edge& edge = network.edges[browsed_edge_rank];
    const int origin_side = edge.side[0].node_rank == origin_node_rank? 0:1;
    const int destination_side = 1-origin_side;

    /*
     * Side whose fee to account
     * If LiquidityDirection = Outbound, we calculate fee for transacton from origin_node to the new one.
     * the fee paid is the one on the destination side of an edge, so on the destination side.
     */
     const int side_for_fee = (testDirection == LiquidityDirection::OUTBOUND? destination_side : origin_side);
     const Edge::Side& fee_side = edge.side[side_for_fee];
     if(fee_side.max_htlc_msat<width_sat*1000ULL)
         return;
     const int64_t edge_cost_msat = fee_side.base_fee_msat + (fee_side.feerate_msat*test_amt_sat)/1000;

     if(current_cost_msat+edge_cost_msat > (max_fee_sat*1000))
     {
         //qWarning()<<"Max cost reached";
         //Max cost reached. we can not cross this edge. Return
         return;
     }
     current_cost_msat += edge_cost_msat;

     //edge crossable.
     //Mark the edge as as reached, and add its capacity to reachable capacity
     reached_edges[browsed_edge_rank] = std::min<uint64_t>(current_cost_msat, reached_edges[browsed_edge_rank]);
     //Add edge capacity as reached capacity

     //If dest node was not already in browsed nodes, we will need to browse its edges next
     const int other_node_rank = edge.side[destination_side].node_rank;
     if(current_cost_msat >= reached_nodes[other_node_rank] )
     {
         //qWarning()<<"Reached node already browsed";
         return;
     }
     //Look at it's edges if we can reach more
     browse_node(other_node_rank, current_cost_msat, width_sat, browsed_edge_rank);
  };

  browse_node(node0_rank, 0, test_amt_sat, -1);
  //qWarning() << reached_sats/100000000.<<"BTC reached";
  qWarning() << network.edges.size() - std::count(reached_edges.begin(), reached_edges.end(), ULONG_LONG_MAX) << "edges reached";
  qWarning() << network.nodes.size() - std::count(reached_nodes.begin(), reached_nodes.end(), ULONG_LONG_MAX) << "nodes reached";

  return;

}
