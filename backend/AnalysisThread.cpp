#include "AnalysisThread.hpp"
#include "Hopness.hpp"
#include <Analyse_capacityPerHop.hpp>
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
void AnalysisThread::analyseHops(const NetworkSummary& networkRef, const int& node0_rank, const Config& config, Result& result)
{
    //Compare 2 results of capacity at each hop leve
    //Returns capacity that is nearer relative to ref
    //Note: asumes ref is before a connection that r2 has, so it can only be better, and r2 has no nodes 'further'
    auto compare_results = [](const QList<qint64>& ref, const QList<qint64>& r2) -> quint64
    {
      qint64 nearer_capacity=0;
      for (int h=2;h<ref.size();++h)
      {
        //if there are less sats on one hop level, they are now nearer
        qint64 new_reach = 0;
        if(r2.size()>h) //we might have one less max hop level afer a connection!
          new_reach = r2[h];
        qint64 delta = new_reach - ref[h];
        if(delta < 0)
          nearer_capacity -= delta;
      }
      return nearer_capacity;
    };


    ReachTree base_reach_tree;
    Hopness base_result;



    //Base capacity for each hop
    capacity_per_hop(base_reach_tree, networkRef, base_result, node0_rank, config);

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

    //Config* config = Config::getInstance();
    QMultiMap<qint64,int> reachable_capacity;
    ReachTree refTree;
    Hopness refResult;
    capacity_per_hop(refTree, networkRef, refResult, node0_rank, config);

    auto compare_candidates = [&config, &compare_results, &refResult, &node0_rank]
            (const std::vector<int>& cand,const NetworkSummary& net_ref, uint64_t& best_nearer_cap, int& best_candidate)
    {
        qWarning() << QString("Analysing %1 candidates").arg(cand.size());
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

        best_candidate = -1;
        best_nearer_cap = 0;
        for(size_t i=0;i<cand.size();++i)
        {
            int node_rank = cand[i];
            const Node& cand_node = net_ref.nodes[node_rank];

            /*if(!net_ref.nodes[node_rank].clearnet)
                continue;*/
            if(cand_node.minChanSize>config.minRoutingCap)
                continue;
            /*if(config.ignored_endpoint_nodes.contains(cand_node.pubKey))
                continue;*/
            if(config.zbfEndpoints && !cand_node.isZbf)
                continue;

            ReachTree tmp_reach_tree;
            Hopness tmp_result;
            capacity_per_hop(tmp_reach_tree, net_ref, tmp_result, node0_rank, config, std::vector<int>(1, node_rank));
            uint64_t nearer_cap = compare_results(refResult.capacities, tmp_result.capacities);
            if(nearer_cap > best_nearer_cap)
            {
                best_nearer_cap = nearer_cap;
                best_candidate = node_rank;
            }
        }
        qWarning() << QString("Best candidate %1 brings %2 BTC nearer")
                      .arg(net_ref.nodes[best_candidate].pubKey)
                      .arg(best_nearer_cap/100000000.0);
    };


    std::thread t2([&](){compare_candidates(base_reach_tree[config.candidates_depth], networkRef, result.cap2, result.node2);});
    std::thread t3([&](){compare_candidates(base_reach_tree[config.candidates_depth+1], networkRef, result.cap3, result.node3);});
      //Do full hops analyse for each candidate
/*
      compare_candidates(base_reach_tree[config->candidates_depth+1], networkRef, result.cap3, result.node3);*/
    t2.join();
    t3.join();

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
  //Result result;
  int node0_rank = network->pubkey_index.value(node0,-1);
  if(node0_rank<0)
      return;

  int test_amt_sat = 1250000;

  Config config;
  config.minRoutingCap = test_amt_sat;
  //analyseHops(*network, node0_rank, config, result);

  //filter twice to remove nodes that have no edges after edges filtering
  const NetworkSummary filtered_network = network->filter(config, node0_rank).filter(config, node0_rank);
  node0_rank = filtered_network.pubkey_index.value(network->nodes[node0_rank].pubKey);
  qWarning()<<"Filtered network has "<<filtered_network.nodes.size()<<" nodes and "<<filtered_network.edges.size()<<" edges";

  CFF_Params outbound_params({filtered_network, config, node0_rank, 500, (uint64_t)test_amt_sat, LiquidityDirection::OUTBOUND});
  CFF_Params inbound_params(outbound_params);
  inbound_params.testDirection = LiquidityDirection::INBOUND;
  CFF_Result inbound_results, outbound_results;
  capacity_for_fee(outbound_params, outbound_results);
  capacity_for_fee(inbound_params, inbound_results);
}

AnalysisThread* AnalysisThread::getInstance()
{
    if(!_instance)
        _instance = new AnalysisThread();
    return _instance;
}
