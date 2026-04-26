#pragma once
#include "Iridium/Analysis/IntraProceduralAnalysis.h"
#include "Iridium/Analysis/DataflowSolver.h"
#include "Iridium/Analysis/Domains/ConstantsAtStmt.h"
#include "Iridium/Analysis/Domains/CopyPropInfo.h"
#include "Iridium/Analysis/Domains/EffectAtStmt.h"
#include "Iridium/Analysis/Domains/Liveness.h"
#include "Iridium/Analysis/Domains/SetSafePropKeyAccesses.h"
#include "Iridium/Analysis/Domains/TDZA.h"
#include "Iridium/Structure/BBContainerView.h"

class ConstantsAtStmtAnalysis : public IntraProceduralAnalysis<ConstantsAtStmt> {
public:
  std::unordered_map<Vertex, ConstantsAtStmt> compute(BBContainerView& view) override {
    auto bl = view.getCapturedStackBindings();
    DataflowSolver<ConstantsAtStmt> solver(view.cfgManager, true,
      [&bl]() { return ConstantsAtStmt::bottom(bl); });
    return solver.run(ConstantsAtStmt::boundary(bl));
  }
};

class CopyPropInfoAnalysis : public IntraProceduralAnalysis<CopyPropInfo> {
public:
  std::unordered_map<Vertex, CopyPropInfo> compute(BBContainerView& view) override {
    auto bl = view.getCapturedStackBindings();
    DataflowSolver<CopyPropInfo> solver(view.cfgManager, true,
      [&bl]() { return CopyPropInfo::make(bl); });
    return solver.run(CopyPropInfo::make(bl));
  }
};

class TDZAAnalysis : public IntraProceduralAnalysis<TDZA> {
public:
  std::unordered_map<Vertex, TDZA> compute(BBContainerView& view) override {
    auto all = view.getAllStackBindings();
    DataflowSolver<TDZA> solver(view.cfgManager, true,
      [&all]() { return TDZA::bottom(all); });
    return solver.run(TDZA::boundary(all));
  }
};

class LivenessAnalysis : public IntraProceduralAnalysis<Liveness> {
public:
  std::unordered_map<Vertex, Liveness> compute(BBContainerView& view) override {
    auto captured = view.getCapturedStackBindings();
    DataflowSolver<Liveness> solver(view.cfgManager, false,
      [&captured]() { return Liveness::bottom(captured); });
    return solver.run(Liveness::boundary(captured, captured));
  }
};

class EffectAtStmtAnalysis : public IntraProceduralAnalysis<EffectAtStmt> {
public:
  std::unordered_map<Vertex, EffectAtStmt> compute(BBContainerView& view) override {
    auto bl = view.getCapturedStackBindings();
    DataflowSolver<EffectAtStmt> solver(view.cfgManager, true,
      [&bl]() { return EffectAtStmt::make(bl); });
    return solver.run(EffectAtStmt::make(bl));
  }
};

class SetSafePropKeyAccessesAnalysis : public IntraProceduralAnalysis<SetSafePropKeyAccesses> {
public:
  std::unordered_map<Vertex, SetSafePropKeyAccesses> compute(BBContainerView& view) override {
    auto uncaptured = view.getUncapturedStackBindings();
    auto bl = view.getCapturedStackBindings();
    DataflowSolver<SetSafePropKeyAccesses> solver(view.cfgManager, true,
      [&uncaptured, &bl]() { return SetSafePropKeyAccesses::bottom(uncaptured, bl); });
    return solver.run(SetSafePropKeyAccesses::boundary(uncaptured, bl));
  }
};
