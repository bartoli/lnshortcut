#include "cff.hpp"
#include <QDebug>
#include <Config.hpp>
#include <thread>
#include <bits/stl_multiset.h>


void CFF_Result::initializeWorkData(const CFF_Params& params)
{
    refReach[0].nodes.resize(params.network.nodes.size(), ULONG_LONG_MAX);
    refReach[1].nodes.resize(params.network.nodes.size(), ULONG_LONG_MAX);
    refReach[0].edges.resize(params.network.edges.size(), false);
    refReach[1].edges.resize(params.network.edges.size(), false);
}

void CFF_Result::initializeResults(const CFF_Params& params)
{
    //After node0 reach analysis, before candidates analysis, init all metrics with node0 as best candidate

    {
      CFF_Metric& metric = metrics[LiquidityDirection::INBOUND][RankingCategory::NEW_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::INBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::INBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::INBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::OUTBOUND][RankingCategory::NEW_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::OUTBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::OUTBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::OUTBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::INBOUND][RankingCategory::CHEAPER_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = refReach[LiquidityDirection::INBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::OUTBOUND][RankingCategory::CHEAPER_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = refReach[LiquidityDirection::OUTBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::CHEAPER_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = INT_MAX;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::INBOUND][RankingCategory::NEW_EDGES];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::INBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::INBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::INBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::OUTBOUND][RankingCategory::NEW_EDGES];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::OUTBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::OUTBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::OUTBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::INBOUND][RankingCategory::MEDIAN_COST];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::INBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::INBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::INBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::OUTBOUND][RankingCategory::MEDIAN_COST];
      metric.rank = params.node0_rank;
      metric.newNodes = refReach[LiquidityDirection::OUTBOUND].reached_nodes;
      metric.newEdges = refReach[LiquidityDirection::OUTBOUND].reached_edges;
      metric.medianCost = refReach[LiquidityDirection::OUTBOUND].median_cost_msat;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = INT_MAX;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_NODES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = INT_MAX;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_EDGES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = INT_MAX;
    }
    {
      CFF_Metric& metric = metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_EDGES];
      metric.rank = params.node0_rank;
      metric.newNodes = 0;
      metric.newEdges = 0;
      metric.medianCost = INT_MAX;
    }
}

/*void CFF_ResultSet::initialize(const CFF_Params& params)
{
    for(int i=0; i< LiquidityDirection::COUNT;++i)
    {
        results[i].initialize(params);
    }
}*/

void browse_edge(const CFF_Params& params, CFF_NodeReach& reach,
        int browsed_edge_rank, int nearer_node_rank,
        uint64_t current_cost_msat, int hop_number);

/*
 * A node has been reached. broswe its edges
 * \param[in] origin_edge_rank  to avoid going back in original direction.
 *            For 'candidate' nodes, set this to an unexisting edge rank for the node so all edges are searched
 */
void browse_node(const CFF_Params& params, CFF_NodeReach& reach,
        int node_rank, uint64_t current_cost_msat, int origin_edge_rank, int hop_number)
{
    /*if(params.testDirection == LiquidityDirection::OUTBOUND)
      Logger::routeLogger->log(QString("Reached node %1. Current_cost=%2, origin_edge=%3").arg(node_rank).arg(current_cost_msat).arg(origin_edge_rank).toUtf8());*/

//#ifdef DEBUG
    //useless iff, otherwise, the edge leading to it wouldn't have been crossed?
    if(current_cost_msat>(params.max_fee_sat*1000))
    {
        throw;
    }
//#endif


    //called only at first reach, or new reach with a lower cost. update new reach cost
    reach.nodes[node_rank] = current_cost_msat;

    if(hop_number>= params.max_hops)
        return;

    const Node& reached_node = params.network.nodes[node_rank];
    //if(current_cost_msat<(max_fee_sat*1000))//useless iff, otherwise, the edge leading to it wouldn't have been crossed?
    {
        //choose edge list ordered in increasing fee order for thetested side, to find cheapest paths first
        auto& edges = params.testDirection == LiquidityDirection::OUTBOUND?
                    reached_node.outbound_edges: reached_node.inbound_edges;

        for(int ie=0, cnt=edges.size();ie<cnt;++ie)
        {
            int edge_rank = edges[ie];
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

            browse_edge(params, reach, edge_rank, node_rank, current_cost_msat, hop_number);
        }
    }
};

//browse a new node and beyond (recursive until a max_cost is reached)
void browse_edge(const CFF_Params& params, CFF_NodeReach& reach,
        int browsed_edge_rank, int nearer_node_rank,
        uint64_t current_cost_msat, int hop_number)
{
  //qWarning() << "Browsing edge "<<browsed_edge_rank;
  const Edge& edge = params.network.edges[browsed_edge_rank];
  const int nearer_side = edge.side[0].node_rank == nearer_node_rank? 0:1;
  /*if(edge.side[nearer_side].node_rank != nearer_node_rank)
      throw;*/
  const int further_side = 1-nearer_side;

  /*
   * Side whose fee to account
   * If LiquidityDirection = Outbound, we calculate fee for transacton from origin_node to the new one.
   * the fee paid is the one on the destination side of an edge, so on the destination side.
   */
   const int side_for_fee = (params.testDirection == LiquidityDirection::INBOUND? further_side : nearer_side);
   const Edge::Side& fee_side = edge.side[side_for_fee];
   const Edge::Side& discount_side = edge.side[1-side_for_fee];
   if(fee_side.disabled)
       return;
   if(fee_side.max_htlc_msat < params.test_amt_sat*1000ULL)
       return;

   int64_t edge_cost_msat = fee_side.base_fee_msat + (fee_side.feerate_msat*params.test_amt_sat)/1000;
   const int64_t edge_discount_msat = discount_side.inbound_base_fee_msat + (discount_side.inbound_feerate_msat*params.test_amt_sat)/1000;
   edge_cost_msat += edge_discount_msat;
   if(edge_cost_msat<0)
       edge_cost_msat;
   if(current_cost_msat+edge_cost_msat > (params.max_fee_sat*1000))
   {
       //qWarning()<<"Max cost reached";
       //Max cost reached. we can not cross this edge. Return
       return;
   }
   current_cost_msat += edge_cost_msat;


   //We do not stop here if edge was already reached with a lower cost.
   //We might have encountered a loop and need to browse the node from the other direction
   //it will stop anyways if it makes us reach a node we already reached for a lower cost
   //BUG: In theory, it should be stopable here, but not stopping makes us reach a few more edges?? TO be confirmed by two runs at same block height
   /*if(current_cost_msat >= result.reached_edges[browsed_edge_rank])
       return;*/

   //edge is crossable from this path.
   //Mark the edge as as reached, and add current cost as reach cost if smaller than previous
   //TODO : calcualte min here if we comment the if before this
   reach.edges[browsed_edge_rank] = true;


   //If dest node was not already in browsed nodes, we will need to browse its edges next
   const int further_node_rank = edge.side[further_side].node_rank;
   //We do not rebrowse the node if it was reached from a different path for the same cost,
   //since it would just reach the same edges
   //then lead to an infinite loop
   if(current_cost_msat >= reach.nodes[further_node_rank] )
   {
       //qWarning()<<"Reached node already browsed";
       return;
   }
   //Look at it's edges if we can reach more
   browse_node(params, reach, further_node_rank, current_cost_msat, browsed_edge_rank, hop_number+1);
};

/*cutom allocator for the set.
 * On  230729, allcation for the set is what takes most time of an estimation
 * We know max set size, and elemnt size.
 * Add a custom allocator with preallocated memory
 */
    //thanks chatGPT!
    template <typename T, std::size_t N>
    class FixedSizeAllocator {
    public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T value_type;


        // Constructor
        FixedSizeAllocator() {}

        // Rebind template for FixedSizeAllocator with different type U
            template <typename U>
            struct rebind {
                using other = FixedSizeAllocator<U, N>;
        };

        // Allocate memory for 'n' elements of type T
        T* allocate(std::size_t n) {
            /*if (n > N) {
                throw std::bad_alloc();
            }
            if (currentSize + n > N) {
                throw std::bad_alloc();
            }*/

            T* ptr = &buffer[currentSize];
            currentSize += n;
            return ptr;
        }

        // Deallocate memory
        void deallocate(T* ptr, std::size_t n) {
            // This allocator doesn't deallocate individual elements, it only resets the current size.
            if (ptr != nullptr && n <= N) {
                currentSize -= n;
                if (currentSize < 0) {
                    currentSize = 0;
                }
            }
        }

    private:
        T buffer[N];       // The fixed-size buffer to hold elements of type T
        int currentSize = 0;  // The current size of the buffer
    };


//Compute reached nodes count, and median cost to reach them
int median_node_reach_cost(const CFF_Params &params,
        const std::vector<uint64_t> &reached_nodes,
        int &reached_nodes_count)
  {
    int median_cost_msat = -1/*, max_cost=-1*/;
        reached_nodes_count = 0;


    std::multiset<uint64_t, std::less<uint64_t>, FixedSizeAllocator<uint64_t,32768>> reach_costs;
    for (int in=0, ncnt = reached_nodes.size(); in<ncnt; ++in)
    {
        if(in == params.node0_rank)
            continue;
        auto cost = reached_nodes[in];
        if(cost >params.max_fee_sat*1000/*== ULONG_LONG_MAX*/)
            continue;
        reach_costs.insert(cost);
    }
    reached_nodes_count = reach_costs.size();
    if(reached_nodes_count>0)
    {
        //std::sort(reach_costs.begin(), reach_costs.end());

        int median_node = reached_nodes_count/2;
        auto i1 = reach_costs.cbegin();
        std::advance(i1, median_node);
        median_cost_msat = *i1;

        //max_cost = *reach_costs.crbegin();
    }
    return median_cost_msat;
};

/*
 *  Evaluate the extra connecton for each candidates
 */
void analyze_candidates(const CFF_Params& params, CFF_Result& result)
{
 //Build list of nodes to avoid evaluating because we are already well enough connected to them
 QSet<int> already_connected_nodes;
 // exclude ourself
 already_connected_nodes.insert(params.node0_rank);
 const Node& node0 = params.network.nodes[params.node0_rank];
 // Exclude peers we are already connected to
 // (should only be done if the channels are better than the wanted one? or a config option?)
 for (const int edge_rank : node0.edges)
 {
    const Edge& edge = params.network.edges[edge_rank];
    // Do not exclude already connected nodes with a smaller channel size than the wanted one
    if(edge.capacity<params.config.minEndpointCap)
        continue;
    const int other_side = (edge.side[0].node_rank==params.node0_rank) ? 1 : 0;
     already_connected_nodes.insert(edge.side[other_side].node_rank);
 }
}

/*void analyze_candidates(const CFF_Params& params, CFF_Result& result)
{
  //Build list of nodes to avoid evaluating
  QSet<int> already_connected_ranks;
  //exclude ourself
  already_connected_ranks.insert(params.node0_rank);
  const Node& node0 = params.network.nodes[params.node0_rank];
  //exclude peers we are already connected to
  //should only be done if the chanels are better than the wanted one? or a config option?
  for (auto edge_rank : node0.edges)
  {
     const Edge& edge = params.network.edges[edge_rank];
     //Do not exclude already connected nodes with a smaller channel size than the wanted one
     if(edge.capacity<params.config.minEndpointCap)
         continue;
     int other_side = edge.side[0].node_rank==params.node0_rank? 1:0;
      already_connected_ranks.insert(edge.side[other_side].node_rank);
  }

  int best_candidate_for_edges = params.node0_rank;
  int best_candidate_for_nodes = params.node0_rank;
  int best_reached_nodes=0;
  int best_reached_nodes2=0;
  int best_reached_edges=0;
  int best_reached_edges2=0;
  int best_candidate_for_cost = params.node0_rank;
  int best_median_cost=result.median_cost_for_original_reach[CFF_Result::RankingCategory::REFERENCE];

  for(size_t reached_node_rank=0; reached_node_rank < result.reached_nodes.size(); ++reached_node_rank)
  {
      if(already_connected_ranks.contains(reached_node_rank)) //TODO? replace by a sentinel value somewhere?
          continue;
      if(params.network.ignored_endpoints.contains(reached_node_rank))
          continue;
      const Node& candidate_node = params.network.nodes[reached_node_rank];
      if(params.config.excludesNodeAsEndPoint(candidate_node, params.network))
          continue;
      if(candidate_node.median_feerate_ppm>1000)
          continue;
      //nodes with smallest channel size above the wanted size wil probably not accept it

      //
      // Estimate new reach for a candidate, assuming channel fees will be the nodes'median fees
      // For OUTBOUND test, first channel fee is the median fee from the original node
      // For INBOUND fee, first channel fee is the median fee of the tested node
      //
      int first_hop_feerate = (params.testDirection == LiquidityDirection::OUTBOUND)? node0.median_feerate_ppm : candidate_node.median_feerate_ppm;
      int first_hop_basefee = (params.testDirection == LiquidityDirection::OUTBOUND)? node0.median_basefee_ppm : candidate_node.median_basefee_ppm;
      int node_reach_cost_msat = first_hop_basefee + (first_hop_feerate*params.test_amt_sat/1000);

      //No need to evaluate the candidate if the cost of the first edge would be higher than the current cost to reach it or the max test cost
      if(node_reach_cost_msat>result.reached_nodes[reached_node_rank] || node_reach_cost_msat>params.max_fee_sat*1000)
          continue;

      CFF_Result new_result(result);
      browse_node(params, new_result,
                  reached_node_rank, node_reach_cost_msat, -1, 1);
      int new_reached_edges_count = std::count(new_result.reached_edges.begin(), new_result.reached_edges.end(), true);
      int new_reached_nodes_count = 0;//params.network.nodes.size() - std::count(new_result.reached_nodes.begin(), new_result.reached_nodes.end(), ULONG_LONG_MAX);

      int new_median_node_reach_cost = median_node_reach_cost(params, new_result.reached_nodes, new_reached_nodes_count);
      if(new_median_node_reach_cost < best_median_cost)
      {
          best_candidate_for_cost = reached_node_rank;
          best_median_cost = new_median_node_reach_cost;
      }

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


};*/ //analyze_candidates()

void NodeReach::preprocess(const CFF_Params& params)
{
  reached_edges = std::count(edges.begin(), edges.end(), true);
  median_cost_msat = median_node_reach_cost(params, nodes, reached_nodes);
}

void NodeReach::preprocessDelta(const CFF_Params&, const NodeReach& refReach)
{
    for(int i=0, cnt = nodes.size();i<cnt;++i)
    {
        if(nodes[i]<refReach.nodes[i])
            cheaperNodes++;
    }
}


void run_CFF(const CFF_Params& params, CFF_Result& result)
{
    //init
    result.initializeWorkData(params);

    //analyze_current_reach
    auto analyze_current_reach = [&](const LiquidityDirection dir)
    {
        CFF_Params p(params);
        p.testDirection = dir;
        //find current reach
        browse_node(p, result.refReach[dir], p.node0_rank, 0, -1, 0);
        //calculate reference metrics
        result.refReach[dir].preprocess(p);
        /*result.metrics[dir][RankingCategory::REFERENCE].rank=params.node0_rank;
        result.metrics[dir][RankingCategory::REFERENCE].value=params.node0_rank;
        result.metrics[dir][RankingCategory::MOST_NEW_NODES].rank=params.node0_rank;
        result.metrics[dir][RankingCategory::MOST_NEW_NODES].value=result.refReach[dir].reached_nodes;
        result.metrics[dir][RankingCategory::MOST_NEW_EDGES].rank=params.node0_rank;
        result.metrics[dir][RankingCategory::MOST_NEW_EDGES].value=result.refReach[dir].reached_edges;*/
    };
    /*std::thread t1([&](){*/analyze_current_reach(LiquidityDirection::OUTBOUND);/*});*/
    /*std::thread t2([&](){*/analyze_current_reach(LiquidityDirection::INBOUND);/*});*/
    /*t1.join();
    t2.join();*/

    result.initializeResults(params);

    //Now loop on candidates
    {
      //Build list of nodes to avoid evaluating because we are already well enough connected to them
      QSet<int> already_connected_nodes;
      // exclude ourself
      already_connected_nodes.insert(params.node0_rank);
      const Node& node0 = params.network.nodes[params.node0_rank];
      // Exclude peers we are already connected to
      // (should only be done if the channels are better than the wanted one? or a config option?)
      for (const int edge_rank : node0.edges)
      {
         const Edge& edge = params.network.edges[edge_rank];
         // Do not exclude already connected nodes with a smaller channel size than the wanted one
         if(edge.capacity<params.config.minEndpointCap)
             continue;
         const int other_side = (edge.side[0].node_rank==params.node0_rank) ? 1 : 0;
          already_connected_nodes.insert(edge.side[other_side].node_rank);
      }

      size_t total_node_count = params.network.nodes.size();
      for(size_t reached_node_rank=0; reached_node_rank < total_node_count; ++reached_node_rank)
      {
          //ignores useless candidates
          if(already_connected_nodes.contains(reached_node_rank)) //TODO? replace by a sentinel value somewhere?
              continue;
          if(params.network.ignored_endpoints.contains(reached_node_rank))
              continue;
          const Node& candidate_node = params.network.nodes[reached_node_rank];
          if(params.config.excludesNodeAsEndPoint(candidate_node, params.network))
              continue;
          if(candidate_node.median_feerate_ppm>1000)
              continue;
          //nodes with smallest channel size above the wanted size will probably not accept it
          //TODO

          //Evaluate the candidate in both directions
          auto evaluate_candidate_reach = [&](const LiquidityDirection dir, NodeReach& newReach)
          {
            CFF_Params p(params);
            p.testDirection = dir;
            const CFF_NodeReach& ref_reach = result.refReach[dir];
            // Estimate new reach for a candidate, assuming channel fees will be the nodes'median fees
            // For OUTBOUND test, first channel fee is the median fee from the original node
            // For INBOUND fee, first channel fee is the median fee of the tested node
            //
            int first_hop_feerate = (dir == LiquidityDirection::OUTBOUND)? node0.median_feerate_ppm : candidate_node.median_feerate_ppm;
            int first_hop_basefee = (dir == LiquidityDirection::OUTBOUND)? node0.median_basefee_ppm : candidate_node.median_basefee_ppm;
            int node_reach_cost_msat = first_hop_basefee + (first_hop_feerate*params.test_amt_sat/1000);

            //No need to evaluate the candidate if the cost of the first edge would be higher than the current cost to reach it or the max test cost
            if(node_reach_cost_msat>ref_reach.nodes[reached_node_rank] || node_reach_cost_msat>params.max_fee_sat*1000)
                return false;

            newReach = ref_reach;
            browse_node(p, newReach,
                        reached_node_rank, node_reach_cost_msat, -1, 1);
            /*int new_reached_edges_count = std::count(new_result.reached_edges.begin(), new_result.reached_edges.end(), true);
            int new_reached_nodes_count = 0;//params.network.nodes.size() - std::count(new_result.reached_nodes.begin(), new_result.reached_nodes.end(), ULONG_LONG_MAX);*/
            newReach.preprocess(p);
            newReach.preprocessDelta(p, ref_reach);

            return true;
          };
          //No multithreading here because it's generally fast enough that we would gain nothing
          CFF_NodeReach cand_reach[2];

          bool outbound_ok = evaluate_candidate_reach(LiquidityDirection::OUTBOUND, cand_reach[LiquidityDirection::OUTBOUND]);
          bool inbound_ok = evaluate_candidate_reach(LiquidityDirection::INBOUND, cand_reach[LiquidityDirection::INBOUND]);
          bool both_ok = (inbound_ok && outbound_ok);

          //Now find if it is a best candidate for each metric
          if(inbound_ok)
          {
            {
              CFF_Metric& metric = result.metrics[LiquidityDirection::INBOUND][RankingCategory::NEW_EDGES];
              const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::INBOUND];
              if(new_reach.reached_edges > metric.newEdges)
              {
                  metric.newEdges = new_reach.reached_edges;
                  metric.newNodes = new_reach.reached_nodes;
                  metric.medianCost = new_reach.median_cost_msat;
                  metric.rank = reached_node_rank;
              }
            }
            {
              CFF_Metric& metric = result.metrics[LiquidityDirection::INBOUND][RankingCategory::NEW_NODES];
              const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::INBOUND];
              if(new_reach.reached_nodes > metric.newNodes)
              {
                  metric.newEdges = new_reach.reached_edges;
                  metric.newNodes = new_reach.reached_nodes;
                  metric.medianCost = new_reach.median_cost_msat;
                  metric.rank = reached_node_rank;
              }
            }
            {
              CFF_Metric& metric = result.metrics[LiquidityDirection::INBOUND][RankingCategory::CHEAPER_NODES];
              const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::INBOUND];
              if(new_reach.cheaperNodes > metric.cheaperNodes)
              {
                  metric.newEdges = new_reach.reached_edges;
                  metric.newNodes = new_reach.reached_nodes;
                  metric.cheaperNodes = new_reach.cheaperNodes;
                  metric.medianCost = new_reach.median_cost_msat;
                  metric.rank = reached_node_rank;
              }
            }
            {
              CFF_Metric& metric = result.metrics[LiquidityDirection::INBOUND][RankingCategory::MEDIAN_COST];
              const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::INBOUND];
              if(new_reach.median_cost_msat < metric.medianCost)
              {
                  metric.newEdges = new_reach.reached_edges;
                  metric.newNodes = new_reach.reached_nodes;
                  metric.medianCost = new_reach.median_cost_msat;
                  metric.rank = reached_node_rank;
              }
            }
          }
          if(outbound_ok)
          {
          {
            CFF_Metric& metric = result.metrics[LiquidityDirection::OUTBOUND][RankingCategory::NEW_EDGES];
            const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::OUTBOUND];
            if(new_reach.reached_edges > metric.newEdges)
            {
             metric.newEdges = new_reach.reached_edges;
             metric.newNodes = new_reach.reached_nodes;
             metric.medianCost = new_reach.median_cost_msat;
             metric.rank = reached_node_rank;
            }
          }
          {
            CFF_Metric& metric = result.metrics[LiquidityDirection::OUTBOUND][RankingCategory::NEW_NODES];
            const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::OUTBOUND];
            if(new_reach.reached_nodes > metric.newNodes)
            {
                metric.newEdges = new_reach.reached_edges;
                metric.newNodes = new_reach.reached_nodes;
                metric.medianCost = new_reach.median_cost_msat;
                metric.rank = reached_node_rank;
            }
          }
          {
            CFF_Metric& metric = result.metrics[LiquidityDirection::OUTBOUND][RankingCategory::CHEAPER_NODES];
            const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::OUTBOUND];
            if(new_reach.cheaperNodes > metric.cheaperNodes)
            {
                metric.newEdges = new_reach.reached_edges;
                metric.newNodes = new_reach.reached_nodes;
                metric.cheaperNodes = new_reach.cheaperNodes;
                metric.medianCost = new_reach.median_cost_msat;
                metric.rank = reached_node_rank;
            }
          }
          {
            CFF_Metric& metric = result.metrics[LiquidityDirection::OUTBOUND][RankingCategory::MEDIAN_COST];
            const CFF_NodeReach& new_reach = cand_reach[LiquidityDirection::OUTBOUND];
            if(new_reach.median_cost_msat < metric.medianCost)
            {
                metric.newEdges = new_reach.reached_edges;
                metric.newNodes = new_reach.reached_nodes;
                metric.medianCost = new_reach.median_cost_msat;
                metric.rank = reached_node_rank;
            }
          }
          }
          if(both_ok)
          {
            {//Worst direction for most nodes
               int new_reached_nodes[LiquidityDirection::DIRECTION_COUNT];
               new_reached_nodes[LiquidityDirection::OUTBOUND] = cand_reach[LiquidityDirection::OUTBOUND].reached_nodes - result.refReach[LiquidityDirection::OUTBOUND].reached_nodes;
               new_reached_nodes[LiquidityDirection::INBOUND]  = cand_reach[LiquidityDirection::INBOUND].reached_nodes - result.refReach[LiquidityDirection::INBOUND].reached_nodes;
               LiquidityDirection worst_direction = new_reached_nodes[LiquidityDirection::OUTBOUND]<new_reached_nodes[LiquidityDirection::INBOUND]? LiquidityDirection::OUTBOUND : LiquidityDirection::INBOUND;
               int worst_new_nodes = new_reached_nodes[worst_direction];
               int best_new_nodes = new_reached_nodes[1-worst_direction];
               CFF_NodeReach& new_reach = cand_reach[worst_direction];
               CFF_Metric& metric = result.metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_NODES];
               if(worst_new_nodes > metric.newNodes || (worst_new_nodes==metric.newNodes && best_new_nodes>metric.newNodes2))
               {
                   metric.newEdges = new_reach.reached_edges;
                   metric.newNodes = worst_new_nodes;
                   metric.newNodes2 = best_new_nodes;
                   metric.medianCost = new_reach.median_cost_msat;
                   metric.rank = reached_node_rank;
               }
            }
            {//Worst direction for most edges
               int new_reached_edges[LiquidityDirection::DIRECTION_COUNT];
               new_reached_edges[LiquidityDirection::OUTBOUND] = cand_reach[LiquidityDirection::OUTBOUND].reached_edges - result.refReach[LiquidityDirection::OUTBOUND].reached_edges;
               new_reached_edges[LiquidityDirection::INBOUND]  = cand_reach[LiquidityDirection::INBOUND].reached_edges - result.refReach[LiquidityDirection::INBOUND].reached_edges;
               LiquidityDirection worst_direction = new_reached_edges[LiquidityDirection::OUTBOUND]<new_reached_edges[LiquidityDirection::INBOUND]? LiquidityDirection::OUTBOUND : LiquidityDirection::INBOUND;
               int worst_new_edges = new_reached_edges[worst_direction];
               int best_new_edges = new_reached_edges[1-worst_direction];
               CFF_NodeReach& new_reach = cand_reach[worst_direction];
               CFF_Metric& metric = result.metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::NEW_EDGES];
               if(worst_new_edges > metric.newNodes || (worst_new_edges==metric.newEdges && best_new_edges>metric.newNodes2))
               {
                   metric.newNodes = new_reach.reached_nodes;
                   metric.newEdges = worst_new_edges;
                   metric.newEdges2 = best_new_edges;
                   metric.medianCost = new_reach.median_cost_msat;
                   metric.rank = reached_node_rank;
               }
            }
            {//worst direction for cheapest nodes
               int cheaper_nodes[LiquidityDirection::DIRECTION_COUNT];
               cheaper_nodes[LiquidityDirection::OUTBOUND] = cand_reach[LiquidityDirection::OUTBOUND].cheaperNodes;
               cheaper_nodes[LiquidityDirection::INBOUND]  = cand_reach[LiquidityDirection::INBOUND].cheaperNodes;
               LiquidityDirection worst_direction = cheaper_nodes[LiquidityDirection::OUTBOUND]<cheaper_nodes[LiquidityDirection::INBOUND]? LiquidityDirection::OUTBOUND : LiquidityDirection::INBOUND;
               int worst_cheaper_nodes = cheaper_nodes[worst_direction];
               int best_cheaper_nodes = cheaper_nodes[1-worst_direction];
               CFF_NodeReach& new_reach = cand_reach[worst_direction];
               CFF_Metric& metric = result.metrics[LiquidityDirection::WORST_DIRECTION][RankingCategory::CHEAPER_NODES];
               if(worst_cheaper_nodes > metric.cheaperNodes || (worst_cheaper_nodes==metric.cheaperNodes && best_cheaper_nodes>metric.cheaperNodes2))
               {
                   metric.newNodes = new_reach.reached_nodes;
                   metric.cheaperNodes = worst_cheaper_nodes;
                   metric.cheaperNodes2 = best_cheaper_nodes;
                   metric.medianCost = new_reach.median_cost_msat;
                   metric.rank = reached_node_rank;
               }
            }
          }

       }//end of candidates loop
    }//end of candidates analysis

}
