#include "AnalysisThread.hpp"
#include "Hopness.hpp"

#include <Config.hpp>
#include <EdgeDirection.hpp>
#include <NetworkSummary.hpp>
#include <PrefetcherThread.hpp>
#include <QMultiMap>
#include <WorkLauncher.hpp>
#include <QDebug>

namespace
{
  //compare two hopness results to decide which chanel to advise
  //strategy? Better if more at first levels? Better for centrality? Average disance?
}

AnalysisThread* AnalysisThread::_instance = nullptr;

AnalysisThread::AnalysisThread()
 : QThread()
{

}


//analyse nodes found for each hop number
void AnalysisThread::analyseHops(const NetworkSummary& networkRef, const QString& node0_pubkey, int min_capacity, Result& result)
{
    //Compare 2 results of capacity at each hop leve
    //Returns capacity that is nearer relative to ref
    //Note: asumes ref is before a connection that r2 has, so it can only be better, and r2 has no nodes 'further'
    auto compare_results = [](const QList<qint64>& ref, const QList<qint64>& r2) -> qint64
    {
      qint64 nearer_capacity=0;
      for (int h=0;h<ref.size();++h)
      {
        //if there are less sats on one hop level, they are now nearer
        qint64 delta = r2[h] - ref[h];
        if(delta < 0)
          nearer_capacity -= delta;
      }
      return nearer_capacity;
    };

    using ReachTree = std::vector<std::vector<qint32>>;
    ReachTree base_reach_tree;
    Hopness base_result;

    //Compute, for each level of hops, the reached capacity
    auto capacity_per_hop = [&](ReachTree& reach_tree, Hopness& result, qint64 min_cap, std::vector<int> testEdges=std::vector<int>()){
        int node0_rank = networkRef.node_index.value(node0_pubkey);
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
              e.node1 = node0_rank;
              e.node2 = testEdges[i];
              e.capacity = min_cap;
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
              if(edge.capacity<min_cap)
                  continue;
              int other_node_rank = edge.node1==node? edge.node2 : edge.node1;

              //const Node& other_node = network.nodes[other_node_rank];
              if(result.edge_capacity_counted[edge_rank] <0)
              {
                result.capacities[search_depth] += edge.capacity;
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

    //Base capacity for each hop
    capacity_per_hop(base_reach_tree, base_result, min_capacity);

    //For each node, count the capacity it can reach in the 'further' direction
    //capacity reachable by


    //Print nodes of last level
    /*for (size_t i=0; i<reach_tree[search_depth-1].size();++i)
    {
        Node& node = nodes[reach_tree[search_depth-1][i]];
        qint64 total_cap = 0;
        for(size_t j=0;j<node.edges.size();++j)
        {
            Edge& edge = edges[node.edges[j]];
            total_cap += edge.capacity;
        }
        printf("%s (%s, %lu edges, %lld sats)\n", node.pubKey.toUtf8().constData(), node.alias.toUtf8().constData(), node.edges.size(), total_cap);

        //debug check ; does it have an edge with the wanted capacity?
        bool good_edge_found = false;
        for (size_t k=0; k<node.edges.size();++k)
        {
          if(edges[node.edges[k]].capacity>= min_edge_capacity)
          {
              good_edge_found = true;
              break;
          }
        }
        if(!good_edge_found)
        {
            printf("Error: node %s does not have good enough edges!", node.pubKey.toUtf8().constEnd());
        }
    }*/
    //Find best candidate to get mist liquidity nearer
    //We look at peers at 4th hop so in the end noone is further than 3 hops

    //calculate reached capacity outwards of a node
    //recursive for further_hops
    /*auto outwards_capacity = [&network, &min_capacity, &result](int node) -> qint64
    {
         //Node& startNode = nodes[node];
         qint64 cap = 0;
         QSet<int>reached_edges;
         std::function<void(int)> reachedEdges;
         reachedEdges = [&reached_edges,&network,&min_capacity,&cap,&reachedEdges,&result](int node_rank)
         {
           const Node&node = network.nodes[node_rank];
           //if(node.edges.size()!=result.edge_directions[node_rank].size())
           //  puts("Bug");
           for (size_t e=0; e<node.edges.size(); ++e)
           {
             int edge_rank = node.edges[e];
             const Edge& edge = network.edges[edge_rank];
             if(edge.capacity<min_capacity)
                 continue;
             if(result.edge_directions[node_rank][e] == EdgeDirection::SAME || result.edge_directions[node_rank][e] == EdgeDirection::FURTHER)
             {
                 if(!reached_edges.contains(edge_rank))
                 {
                     int other_node_rank = edge.node1==node_rank? edge.node2 : edge.node1;
                     reached_edges.insert(edge_rank);
                     cap += edge.capacity;
                     reachedEdges(other_node_rank);
                 }
             }
           }
         };
         reachedEdges(node);
         return cap;
    };*/

    Config* config = Config::getInstance();
    QMultiMap<qint64,int> reachable_capacity;
    ReachTree refTree;
    Hopness refResult;
    capacity_per_hop(refTree, refResult, min_capacity);
    {
      std::vector<int>& candidates = base_reach_tree[config->candidates_depth];
      qWarning() << QString("Analysing %1 candidates").arg(candidates.size());
      /*for(size_t i=0;i<candidates.size();++i)
      {
          //TODO : use 4 threads to make this part faster? a mutex on the result insert should be enough?
          if(network.nodes[candidates[i]].clearnet && network.nodes[candidates[i]].min_chan_size<=min_capacity&& !config->ignored_endpoint_nodes.contains(network.nodes[candidates[i]].pubKey))
            reachable_capacity.insert(outwards_capacity(candidates[i]), candidates[i]);
      }
      int chosen_node_rank =reachable_capacity.last();
      qWarning() << QString("Best candidate %1 brings %2 sats nearer")
                    .arg(network.nodes[chosen_node_rank].pubKey)
                    .arg(reachable_capacity.lastKey());*/

      //Do full hops analyse for each candidate
      int best_candidate=-1;
      qint64 best_capacity = 0;
      qint64 best_nearer_cap = 0;

      for(size_t i=0;i<candidates.size();++i)
      {
          if(networkRef.nodes[candidates[i]].clearnet && networkRef.nodes[candidates[i]].minChanSize<=min_capacity&& !config->ignored_endpoint_nodes.contains(networkRef.nodes[candidates[i]].pubKey))
          {
            ReachTree tmp_reach_tree;
            Hopness tmp_result;
            capacity_per_hop(tmp_reach_tree, tmp_result, min_capacity, std::vector<int>(1,candidates[i]));
            qint64 nearer_cap = compare_results(refResult.capacities, tmp_result.capacities);
            if(nearer_cap > best_nearer_cap)
            {
                best_nearer_cap = nearer_cap;
                best_candidate = i;
            }
          }
      }
      qWarning() << QString("Best candidate at 2 hops %1 brings %2 BTC nearer")
                    .arg(networkRef.nodes[candidates[best_candidate]].pubKey)
                    .arg(best_nearer_cap/100000000.0);
      result.node2 = candidates[best_candidate];
      result.cap2 = best_nearer_cap;
    }

    {
    std::vector<int>& candidates = base_reach_tree[config->candidates_depth+1];
    qWarning() << QString("Analysing %1 candidates").arg(candidates.size());
    /*for(size_t i=0;i<candidates.size();++i)
    {
        //TODO : use 4 threads to make this part faster? a mutex on the result insert should be enough?
        if(network.nodes[candidates[i]].clearnet && network.nodes[candidates[i]].min_chan_size<=min_capacity&& !config->ignored_endpoint_nodes.contains(network.nodes[candidates[i]].pubKey))
          reachable_capacity.insert(outwards_capacity(candidates[i]), candidates[i]);
    }
    int chosen_node_rank =reachable_capacity.last();
    qWarning() << QString("Best candidate %1 brings %2 sats nearer")
                  .arg(network.nodes[chosen_node_rank].pubKey)
                  .arg(reachable_capacity.lastKey());*/

    //Do full hops analyse for each candidate
    int best_candidate=-1;
    qint64 best_capacity = 0;
    qint64 best_nearer_cap = 0;
    for(size_t i=0;i<candidates.size();++i)
    {
        if(networkRef.nodes[candidates[i]].clearnet && networkRef.nodes[candidates[i]].minChanSize<=min_capacity&& !config->ignored_endpoint_nodes.contains(networkRef.nodes[candidates[i]].pubKey))
        {
          ReachTree tmp_reach_tree;
          Hopness tmp_result;
          capacity_per_hop(tmp_reach_tree, tmp_result, min_capacity, std::vector<int>(1,candidates[i]));
          qint64 nearer_cap = compare_results(refResult.capacities, tmp_result.capacities);
          if(nearer_cap > best_nearer_cap)
          {
              best_nearer_cap = nearer_cap;
              best_candidate = i;
          }
        }
    }
    qWarning() << QString("Best candidate at 3 hops %1 brings %2 BTC nearer")
                  .arg(networkRef.nodes[candidates[best_candidate]].pubKey)
                  .arg(best_nearer_cap/100000000.0);
    result.node3 = candidates[best_candidate];
    result.cap3 = best_nearer_cap;
    }

    //find at which hop depth a node is
    /*
    auto find_node_depth = [&](int node_rank) -> int
    {
      for (size_t d=0;d<reach_tree.size();++d)
      {
          if(std::find(reach_tree[d].begin(), reach_tree[d].end(), node_rank)!=reach_tree[d].end())
              return d+1;
      }
      return -1;
    };
    printf("Chosen node rank : %d\n", find_node_depth(chosen_node_rank));*/
}

void AnalysisThread::run()
{}

/*void AnalysisThread::registerLauncher(WorkLauncher* launcher)
{
    connect(launcher, &WorkLauncher::newWorkRequest, &AnalysisThread::newWork);
}*/

void AnalysisThread::newWork(const QString& node0)
{
   NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
   if(!network)
     return;
  Result result;
  analyseHops(*network, node0, 2500000, result);
}

void AnalysisThread::fetchNodeInfo(const QString& pubkey, int workId)
{
    NetworkSummary* network = PrefetcherThread::getInstance()->_currentNetwork;
    if(!network)
      return;


}

AnalysisThread* AnalysisThread::getInstance()
{
    if(!_instance)
        _instance = new AnalysisThread();
    return _instance;
}
