#pragma once
#include "Iridium/BBPass.h"
#include "Iridium/AnalysisManager.h"
#include "Iridium/Structure/BBContainerView.h"
#include "Iridium/Structure/FileView.h"
#include "Iridium/OptimizationPasses/ConstantProp.h"
#include "Iridium/OptimizationPasses/CopyProp.h"
#include "Iridium/OptimizationPasses/DCE.h"
#include "Iridium/OptimizationPasses/DeadBindingRemoval.h"
#include "Iridium/OptimizationPasses/PropEffects.h"
#include "Iridium/OptimizationPasses/ReduceComputedFieldOps.h"
#include "Iridium/OptimizationPasses/RemoveRedundantPropKeyCast.h"
#include "Iridium/OptimizationPasses/WriteBarrierReduction.h"
#include "Iridium/CorePasses/3_filterNops.h"
#include "Iridium/Analysis/Domains/ConstantsAtStmt.h"
#include "Iridium/Analysis/Domains/CopyPropInfo.h"
#include "Iridium/Analysis/Domains/EffectAtStmt.h"
#include "Iridium/Analysis/Domains/Liveness.h"
#include "Iridium/Analysis/Domains/SetSafePropKeyAccesses.h"
#include "Iridium/Analysis/Domains/TDZA.h"
#include <boost/graph/graph_traits.hpp>
#include <boost/range/iterator_range.hpp>

class ConstantPropPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    auto& constantsMap = am.get<ConstantsAtStmt>(view);
    bool changed = false;
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg)))
      changed |= ConstantProp::Transform(view.cfgManager.cfg[v], constantsMap.at(v));
    return changed;
  }
  std::string name() const override { return "ConstantProp"; }
};

class CopyPropPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    auto& copyMap = am.get<CopyPropInfo>(view);
    bool changed = false;
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg)))
      changed |= CopyProp::Transform(view.cfgManager.cfg[v], copyMap.at(v));
    return changed;
  }
  std::string name() const override { return "CopyProp"; }
};

class WriteBarrierReductionPass : public BBPass {
  FileView& fileView;
public:
  explicit WriteBarrierReductionPass(FileView& fv) : fileView(fv) {}

  bool run(BBContainerView& view, AnalysisManager& am) override {
    WriteBarrierReduction::currFileView = &fileView;
    auto& tdzaMap = am.get<TDZA>(view);
    bool changed = false;
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg)))
      changed |= WriteBarrierReduction::Transform(view.cfgManager.cfg[v], tdzaMap.at(v));
    return changed;
  }
  std::string name() const override { return "WriteBarrierReduction"; }
};

class DCEPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    auto& livenessMap = am.get<Liveness>(view);
    bool changed = false;
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg))) {
      auto bb = view.cfgManager.cfg[v];
      changed |= DCE::Transform(bb, livenessMap.at(v));
      filterNOPs(bb);
    }
    return changed;
  }
  std::string name() const override { return "DCE"; }
};

class PropEffectsPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    auto& livenessMap = am.get<Liveness>(view);
    auto& effectMap   = am.get<EffectAtStmt>(view);
    std::set<IRISEXP> killset;

    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg)))
      PropEffects::Transform(view.cfgManager.cfg[v], killset, livenessMap.at(v), effectMap.at(v));

    if (killset.empty()) return false;

    // Remove all statements that were moved
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg))) {
      auto bb = view.cfgManager.cfg[v];
      auto& args = bb->args;
      args.erase(std::remove_if(args.begin(), args.end(),
        [&](const IRISEXP& s) { return killset.count(s) > 0; }), args.end());
    }
    return true;
  }
  std::string name() const override { return "PropEffects"; }
};

class RemoveRedundantPropKeyCastPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    auto& safePropMap = am.get<SetSafePropKeyAccesses>(view);
    bool changed = false;
    for (auto v : boost::make_iterator_range(boost::vertices(view.cfgManager.cfg)))
      changed |= RemoveRedundantPropKeyCast::Transform(view.cfgManager.cfg[v], safePropMap.at(v));
    return changed;
  }
  std::string name() const override { return "RemoveRedundantPropKeyCast"; }
};

class ReduceComputedFieldOpsPass : public BBPass {
public:
  bool run(BBContainerView& view, AnalysisManager& am) override {
    return reduceComputedFieldOps(view);
  }
  std::string name() const override { return "ReduceComputedFieldOps"; }
};
