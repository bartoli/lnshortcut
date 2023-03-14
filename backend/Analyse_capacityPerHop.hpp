#ifndef ANALYSE_CAPACITYPERHOP_HPP
#define ANALYSE_CAPACITYPERHOP_HPP

#include <vector>
#include <stdint.h>

using ReachTree = std::vector<std::vector<int32_t>>;
class Hopness;
class NetworkSummary;
class Config;


void capacity_per_hop(ReachTree& reach_tree, const NetworkSummary& networkRef, Hopness& result, int node0_rank, const Config& config, std::vector<int> testEdges=std::vector<int>());

#endif // ANALYSE_CAPACITYPERHOP_HPP
