#include <NetworkSummary.hpp>
#include <Config.hpp>

/*
 * Returns a pre-filtered network summary,
 * to speed up analysies
 */
NetworkSummary NetworkSummary::filter(const Config& cfg, int node0_rank) const
{
    /*
     * First filter edges.
     * We remove edges we don't want to browse AND connect too
     * Nodes that are forbiden as endpoints can srill be used in path analyses
     * We store
     */
    const NetworkSummary& ref(*this);
    const size_t n_cnt = ref.nodes.size();
    const size_t e_cnt = ref.edges.size();
    size_t n_cnt2 = n_cnt;
    size_t e_cnt2 = e_cnt;

    std::vector<bool> kept_nodes(n_cnt, true);
    std::vector<bool> kept_edges(e_cnt, true);

    //First loops to filter nodes. Actual nodes filling is later
    for (size_t in=0; in<n_cnt; ++in)
    {
        if(in == node0_rank)
            continue;
        const Node& node = ref.nodes.at(in);
        if(cfg.excludesNodeForRouting(node))
        {
            //First exclusion of edges that are connected to nodes that are excluded
            kept_nodes[in] = false;
            n_cnt2--;
            for(int ie=0, cnt=node.edges.size(); ie<cnt; ++ie)
            {
                if(!kept_edges.at(node.edges.at(ie)))
                    continue;
                kept_edges.at(node.edges.at(ie)) = false;
                e_cnt2--;
            }
        }
    }
    for(size_t ie=0; ie<e_cnt; ++ie)
    {
        if(!kept_edges.at(ie))
            continue;
        if(cfg.excludesEdge(ref, ref.edges.at(ie)))
        {
            kept_edges.at(ie) = false;
            e_cnt2--;
        }
    }

    //TODO (PERF) : merge kept ranks and new ranks, -1 leaning not kept ranks?
    NetworkSummary out;
    std::vector<int> new_node_ranks(n_cnt,-1);
    std::vector<int> new_edge_ranks(e_cnt,-1);
    out.nodes.resize(n_cnt2);
    out.edges.resize(e_cnt2);

    //fill node objects, except for edges (that need updated nodes ranks)
    for (size_t in=0, in2=0; in<n_cnt; ++in)
    {
      if(!kept_nodes.at(in))
          continue;
      Node& node = out.nodes.at(in2);
      node = ref.nodes.at(in);
      node.edges.clear();
      new_node_ranks.at(in) = in2;
      in2++;
    }

    //Repalce new node_ranks in edges
    for(size_t ie=0, ie2=0; ie<e_cnt;++ie)
    {
        if(!kept_edges.at(ie))
            continue;
        const Edge& edge0 = ref.edges.at(ie);
        Edge& edge= out.edges.at(ie2);
        edge = edge0;
        edge.side[0].node_rank = new_node_ranks.at(edge0.side[0].node_rank);
        edge.side[1].node_rank = new_node_ranks.at(edge0.side[1].node_rank);

        new_edge_ranks.at(ie) = ie2;
        ie2++;
    };
    //now replace edges ranks in nodes (and remove obeolete edges
    for (size_t in=0; in<n_cnt; ++in)
    {
        if(!kept_nodes.at(in))
            continue;
        const Node& node0 = ref.nodes.at(in);
        auto new_rank = new_node_ranks.at(in);
        Node& node2 = out.nodes.at(new_rank);
        out.node_index[node0.pubKey] = new_rank;
        for(int ie=0, cnt = node0.edges.size(); ie<cnt; ++ie)
        {
            int edge_rank0 = node0.edges.at(ie);
            if(!kept_edges.at(edge_rank0))
                continue;
            int edge_rank2 = new_edge_ranks.at(edge_rank0);
            node2.edges.push_back(edge_rank2);
        }
    }

    out.updateIgnoredEndpoints(cfg);

    return out;
}

void NetworkSummary::updateIgnoredEndpoints(const Config& cfg)
{
  ignored_endpoints.clear();
  //First fill based on known node min channel size VS config wanted channel size
  auto it( cfg.min_chan_size_db.lowerBound(cfg.minCap) );
  while( it != cfg.min_chan_size_db.end())
  {
    ignored_endpoints.insert(node_index[it.value()]);
    ++it;
  }
}
