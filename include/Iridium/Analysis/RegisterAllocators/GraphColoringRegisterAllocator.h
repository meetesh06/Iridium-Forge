#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include "Iridium/Analysis/RegisterAllocators/RegisterAllocator.h"
#include "Iridium/Analysis/RegisterAllocators/RAContext.h"
#include "generated/IridiumTypes.h"

// Chaitin-style graph coloring register allocator.
//
// Builds an interference graph from liveness results, then runs the
// simplify/select phases to assign register indices to each binding.
// TODO: Change it to use Brigg's algorithm
class GraphColoringRegisterAllocator : public RegisterAllocator
{
public:
  void run(const RAContext &ctx) override;

  size_t getMaxIdx() const override { return maxIdx; }
  void setMaxIdx(size_t idx) override { maxIdx = idx; }

private:
  void addNode(std::shared_ptr<EnvBindingSEXP> n);
  void addEdge(std::shared_ptr<EnvBindingSEXP> u, std::shared_ptr<EnvBindingSEXP> v);
  void color();

  std::set<std::shared_ptr<EnvBindingSEXP>> nodes;
  std::unordered_map<std::shared_ptr<EnvBindingSEXP>, std::set<std::shared_ptr<EnvBindingSEXP>>> edges;
  std::unordered_map<std::shared_ptr<EnvBindingSEXP>, size_t> allocation;

  size_t K = 4;
  size_t spillIdx = 4;
  size_t maxIdx = 0;
};
