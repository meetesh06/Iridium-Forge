#include "Iridium/Analysis/RegisterAllocators/ScopeBasedRegisterAllocator.h"
#include <cassert>

void ScopeBasedRegisterAllocator::run(const RAContext &ctx)
{
  walkScope(ctx.bindingsView, ctx.bindingsView.root);
}

void ScopeBasedRegisterAllocator::walkScope(const BindingsView &bv, double scopeIdx)
{
  scopeStack.push_back(cursor);

  auto bindIt = bv.bindings.find(scopeIdx);
  if (bindIt != bv.bindings.end())
  {
    for (auto &binding : bindIt->second)
    {
      if (binding->hasJSARG() || binding->hasJSRESTARG())
        continue;
      binding->setREFIDX(cursor);
      if (cursor > maxIdx)
        maxIdx = cursor;
      ++cursor;
    }
  }

  auto nextIt = bv.next.find(scopeIdx);
  if (nextIt != bv.next.end())
  {
    for (double childScope : nextIt->second)
      walkScope(bv, childScope);
  }

  cursor = scopeStack.back();
  scopeStack.pop_back();
}
