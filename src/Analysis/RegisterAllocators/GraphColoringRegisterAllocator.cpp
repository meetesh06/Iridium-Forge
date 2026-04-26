#include "Iridium/Analysis/RegisterAllocators/GraphColoringRegisterAllocator.h"

void GraphColoringRegisterAllocator::run(const RAContext &ctx)
{
  // Build interference graph: any two variables simultaneously live interfere.
  for (auto &[vertex, liveness] : ctx.livenessResult)
  {
    auto &bb = ctx.cfgManager.cfg[vertex];
    liveness.iter(bb, [&](size_t /*idx*/, const Liveness &val)
    {
      for (auto it1 = val.dfv.begin(); it1 != val.dfv.end(); ++it1)
      {
        if ((*it1)->hasJSARG() || (*it1)->hasJSRESTARG()) continue;
        addNode(*it1);
        for (auto it2 = val.dfv.begin(); it2 != val.dfv.end(); ++it2)
        {
          if ((*it2)->hasJSARG() || (*it2)->hasJSRESTARG()) continue;
          addNode(*it2);
          if (*it1 != *it2)
            addEdge(*it1, *it2);
        }
      }
    });
  }

  color();
}

void GraphColoringRegisterAllocator::addNode(std::shared_ptr<EnvBindingSEXP> n)
{
  nodes.insert(n);
}

void GraphColoringRegisterAllocator::addEdge(std::shared_ptr<EnvBindingSEXP> u, std::shared_ptr<EnvBindingSEXP> v)
{
  addNode(u);
  addNode(v);
  edges[u].insert(v);
  edges[v].insert(u);
}

void GraphColoringRegisterAllocator::color()
{
  std::vector<std::shared_ptr<EnvBindingSEXP>> stack;

  // --- Simplify phase: peel off low-degree nodes ---
  std::unordered_map<std::shared_ptr<EnvBindingSEXP>, std::set<std::shared_ptr<EnvBindingSEXP>>> graph = edges;
  std::set<std::shared_ptr<EnvBindingSEXP>> remaining(nodes.begin(), nodes.end());

  while (!remaining.empty())
  {
    bool removed = false;
    for (auto it = remaining.begin(); it != remaining.end();)
    {
      auto node = *it;
      if (graph[node].size() < K)
      {
        stack.push_back(node);
        for (auto &nbr : graph[node])
          graph[nbr].erase(node);
        graph.erase(node);
        it = remaining.erase(it);
        removed = true;
        break;
      }
      else
      {
        ++it;
      }
    }

    if (!removed)
    {
      // All remaining nodes have degree >= K: force-spill a candidate.
      auto node = *remaining.begin();
      stack.push_back(node);
      for (auto &nbr : graph[node])
        graph[nbr].erase(node);
      graph.erase(node);
      remaining.erase(node);
    }
  }

  // --- Select phase: assign colors from the stack ---
  while (!stack.empty())
  {
    auto node = stack.back();
    stack.pop_back();

    std::set<size_t> used;
    for (auto &nbr : edges[node])
    {
      if (allocation.count(nbr))
        used.insert(allocation[nbr]);
    }

    size_t assigned = K; // sentinel: no free register found yet
    for (size_t r = 0; r < K; ++r)
    {
      if (!used.count(r))
      {
        assigned = r;
        break;
      }
    }

    if (assigned == K)
      assigned = spillIdx++;

    if (assigned > maxIdx)
      maxIdx = assigned;

    allocation[node] = assigned;
    node->setREFIDX(assigned);
  }
}
