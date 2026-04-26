#pragma once
#include <memory>
#include <vector>
#include "Iridium/Analysis/RegisterAllocators/RegisterAllocator.h"
#include "Iridium/Analysis/RegisterAllocators/RAContext.h"
#include "generated/IridiumTypes.h"

// Scope-based register allocator.
//
// Walks the scope tree from the root, assigning the next free register index
// to each binding as it is declared. On scope exit the cursor rewinds,
// so sibling scopes recycle each other's register indices.
class ScopeBasedRegisterAllocator : public RegisterAllocator
{
public:
  void run(const RAContext &ctx) override;

  size_t getMaxIdx() const override { return maxIdx; }
  void setMaxIdx(size_t idx) override { maxIdx = idx; }

private:
  void walkScope(const BindingsView &bv, double scopeIdx);

  size_t cursor = 0;
  size_t maxIdx = 0;
  std::vector<size_t> scopeStack;
};
