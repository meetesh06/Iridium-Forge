#pragma once
#include "Iridium/Analysis/Domains/SetSafePropKeyAccesses.h"

class RemoveRedundantPropKeyCast
{

  static void patchExpr(IRISEXP curr, SetSafePropKeyAccesses &flowVal, bool &changed)
  {
    if (auto computedFieldRead = std::dynamic_pointer_cast<JSComputedFieldReadSEXP>(curr))
    {
      bool shouldBeSafe = PropKeyLatticeValue::isSafe(computedFieldRead->getField(), flowVal.dfv);
      if (shouldBeSafe && !computedFieldRead->hasSAFE()) {
        computedFieldRead->setSAFE();
        changed = true;
      } else if (!shouldBeSafe && computedFieldRead->hasSAFE()) {
        computedFieldRead->unsetSAFE();
        changed = true;
      }
    }

    if (auto computedFieldWrite = std::dynamic_pointer_cast<JSComputedFieldWriteSEXP>(curr))
    {
      bool shouldBeSafe = PropKeyLatticeValue::isSafe(computedFieldWrite->getField(), flowVal.dfv);
      if (shouldBeSafe && !computedFieldWrite->hasSAFE()) {
        computedFieldWrite->setSAFE();
        changed = true;
      } else if (!shouldBeSafe && computedFieldWrite->hasSAFE()) {
        computedFieldWrite->unsetSAFE();
        changed = true;
      }
    }
    for (auto &e : curr->args)
      patchExpr(e, flowVal, changed);
  }

public:
  static bool Transform(std::shared_ptr<BBSEXP> &bb, SetSafePropKeyAccesses inData)
  {
    bool changed = false;
    inData.iter(bb, [&](size_t idx, SetSafePropKeyAccesses val)
                { patchExpr(bb->args.at(idx), val, changed); });
    return changed;
  }
};