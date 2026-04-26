#pragma once
#include "Iridium/Analysis/Domains/CopyPropInfo.h"

class CopyProp
{
  static std::shared_ptr<EnvBindingSEXP>
  resolve(const std::shared_ptr<EnvBindingSEXP> &start, const CopyPropInfo &val)
  {
    std::set<std::shared_ptr<EnvBindingSEXP>> alreadyVisited;
    auto curr = start;

    while (true)
    {
      // cycle detected → bail out, return original
      if (alreadyVisited.count(curr) > 0)
        return start;

      alreadyVisited.insert(curr);

      // no further mapping → stop
      auto it = val.dfv.find(curr);
      if (it == val.dfv.end())
        return curr;

      curr = it->second;
    }
  }

  static void patchExpr(IRISEXP curr, const CopyPropInfo &val, bool &changed)
  {
    if (auto envReadSEXP = std::dynamic_pointer_cast<EnvReadSEXP>(curr))
    {
      if (auto o = std::dynamic_pointer_cast<EnvBindingSEXP>(envReadSEXP->getObj()))
      {
        if (val.dfv.count(o) > 0)
        {
          auto res = resolve(o, val);
          if (res != o) {
            envReadSEXP->setObj(res);
            changed = true;
          }
        }
      }
    }

    for (auto &e : curr->args)
      patchExpr(e, val, changed);
  }

public:
  static bool Transform(std::shared_ptr<BBSEXP> &bb, CopyPropInfo inData)
  {
    bool changed = false;
    inData.iter(bb, [&](size_t idx, const CopyPropInfo &val)
                { patchExpr(bb->args.at(idx), val, changed); });
    return changed;
  }
};