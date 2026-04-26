#include "Iridium/OptimizationPasses/ReduceComputedFieldOps.h"
#include <cmath>

bool hasFractionalPart(double value) {
    return std::trunc(value) != value;
}
//
// Reduce JSComputedField operations into Field operations if possible
//

static bool patchExpr(IRISEXP currSEXP)
{
  bool changed = false;
  for (size_t i = 0; i < currSEXP->args.size(); i++) {
    auto curr = currSEXP->args.at(i);
    if (auto computedFieldRead = std::dynamic_pointer_cast<JSComputedFieldReadSEXP>(curr))
    {
      auto currField = computedFieldRead->getField();
      if (auto booleanField = std::dynamic_pointer_cast<BooleanSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldReadSEXP>(
          computedFieldRead->getObj(),
          std::make_shared<StringSEXP>(booleanField->getIridiumPrimitive() ? "true" : "false")
        );
        changed = true;
      }
      else if (auto nullObj = std::dynamic_pointer_cast<NullSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldReadSEXP>(
          computedFieldRead->getObj(),
          std::make_shared<StringSEXP>("null")
        );
        changed = true;
      }
      else if (auto stringObj = std::dynamic_pointer_cast<StringSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldReadSEXP>(
          computedFieldRead->getObj(),
          std::make_shared<StringSEXP>(stringObj->getIridiumPrimitive())
        );
        changed = true;
      }
    }

    if (auto computedFieldWrite = std::dynamic_pointer_cast<JSComputedFieldWriteSEXP>(curr))
    {
      auto currField = computedFieldWrite->getField();
      if (auto booleanField = std::dynamic_pointer_cast<BooleanSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldWriteSEXP>(
          computedFieldWrite->getObj(),
          std::make_shared<StringSEXP>(booleanField->getIridiumPrimitive() ? "true" : "false"),
          computedFieldWrite->getValue()
        );
        changed = true;
      }
      else if (auto nullObj = std::dynamic_pointer_cast<NullSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldWriteSEXP>(
          computedFieldWrite->getObj(),
          std::make_shared<StringSEXP>("null"),
          computedFieldWrite->getValue()
        );
        changed = true;
      }
      else if (auto stringObj = std::dynamic_pointer_cast<StringSEXP>(currField))
      {
        currSEXP->args.at(i) = std::make_shared<FieldWriteSEXP>(
          computedFieldWrite->getObj(),
          std::make_shared<StringSEXP>(stringObj->getIridiumPrimitive()),
          computedFieldWrite->getValue()
        );
        changed = true;
      }
    }

    changed |= patchExpr(curr);
  }
  return changed;
}

bool reduceComputedFieldOps(BBContainerView &bbContView)
{
  bool changed = false;
  bbContView.cfgManager.traverseCFG(
  [&](Vertex v, std::shared_ptr<BBSEXP> bb)
  {
    for (auto &stmt : bb->args)
      changed |= patchExpr(stmt);
  });
  return changed;
}
