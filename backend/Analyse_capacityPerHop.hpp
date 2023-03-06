#ifndef ANALYSE_CAPACITYPERHOP_HPP
#define ANALYSE_CAPACITYPERHOP_HPP

#include <vector>
#include <stdint.h>

using ReachTree = std::vector<std::vector<int32_t>>;
class Hopness;
class NetworkSummary;


void capacity_per_hop(ReachTree& reach_tree, const NetworkSummary& networkRef, Hopness& result, int node0_rank, int64_t min_cap, std::vector<int> testEdges=std::vector<int>());

#endif // ANALYSE_CAPACITYPERHOP_HPP
