#pragma once
#include <unordered_map>
#include "Iridium/Structure/CFGManager.h"
#include "Iridium/Analysis/Domains/Liveness.h"
#include "Iridium/Structure/BindingsView.h"

// Bundles all IR context that register allocators may need.
// Each allocator pulls only what is relevant to its strategy.
struct RAContext
{
  const std::unordered_map<Vertex, Liveness> &livenessResult;
  CFGManager &cfgManager;
  const BindingsView &bindingsView;
};
