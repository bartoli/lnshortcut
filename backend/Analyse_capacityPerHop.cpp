
#include <Analyse_capacityPerHop.hpp>
#include <Config.hpp>
#include <Hopness.hpp>
#include <NetworkSummary.hpp>

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
                      int node0_rank,
                      uint64_t max_fee_msat, uint64_t basefee_msat, uint64_t feerate_msat, const LiquidityDirection& testDirection)
{
  // already reached nodes/edges for this browse
  QVector<bool> reached_nodes(network.nodes.size(), false);
  QVector<bool> reached_edges(network.edges.size(), false);

  //recursive browse lambda.
  //Called for each edge of a node.
  //It will call itself while a path contains not yet used edges and total fee from origin to las node is not the max one
  uint64_t current_cost_mstat = 0;

  //browse a new node and beyond (recursive until a max_cost is reached
  auto browse_node = [&network,&reached_nodes, &reached_edges](int origin_node_rank, int browsed_edge_rank,
          qint64 current_cost_msat, uint64_t max_cost_msat,
          LiquidityDirection tested_direction)
  {
    const Edge edge = network.edges[browsed_edge_rank];
    const Node& node0 = network.nodes[origin_node_rank];
    const int origin_side = edge.side[0].node_rank == origin_node_rank? 0:1;
    const int destination_side = 1-origin_side;
    const int other_node_rank = edge.side[destination_side].node_rank;
    const Node& node1 = network.nodes[other_node_rank];


    /*
     * Side that pays the fee.
     * If LiquidityDirection = Outbound, we calculate fee for transacton from origin_node to the new one.
     * the fee paid is the one on the destination side of an edge, so on the destination side.
     */
     /*int side_for_estimaton =


    int64_t edge_cost;*/


  };

}
