#include "Iridium/PassManager.h"
#include "Iridium/Analysis/Analyses.h"
#include "Iridium/OptimizationPasses/Passes.h"
#include "Iridium/OptimizationPasses/DeadBindingRemoval.h"
#include "external/json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <chrono>

using json = nlohmann::json;

static void printOptimizationStatus() {
  struct Flag { const char* env; const char* name; };
  Flag flags[] = {
    {"NO_CONSTPROP",  "CONSTPROP"},
    {"NO_COPYPROP",   "COPYPROP"},
    {"NO_WBR",        "WBR"},
    {"NO_DCE",        "DCE"},
    {"NO_EPROP",      "EPROP"},
    {"NO_RKEYCAST",   "RKEYCAST"},
    {"NO_REDKEYCAST", "REDKEYCAST"},
    {"NO_DEADBR",     "DEADBR"},
  };
  for (const auto& f : flags)
    std::cerr << f.name << ": " << (std::getenv(f.env) ? "OFF" : "ON") << "\n";
}

PassManager::PassManager(FileView& fs, std::unordered_map<int, IRIBUILDCONTEXT> bc)
    : fileView(fs), iridiumBuildContext(std::move(bc))
{
  // Register all analyses
  analysisManager.registerAnalysis<ConstantsAtStmt>(std::make_unique<ConstantsAtStmtAnalysis>());
  analysisManager.registerAnalysis<CopyPropInfo>(std::make_unique<CopyPropInfoAnalysis>());
  analysisManager.registerAnalysis<TDZA>(std::make_unique<TDZAAnalysis>());
  analysisManager.registerAnalysis<Liveness>(std::make_unique<LivenessAnalysis>());
  analysisManager.registerAnalysis<EffectAtStmt>(std::make_unique<EffectAtStmtAnalysis>());
  analysisManager.registerAnalysis<SetSafePropKeyAccesses>(std::make_unique<SetSafePropKeyAccessesAnalysis>());

  // Build default pipeline, respecting the same NO_* env-var knobs as before
  if (!std::getenv("NO_CONSTPROP"))
    pipeline.push_back(std::make_unique<ConstantPropPass>());
  if (!std::getenv("NO_COPYPROP"))
    pipeline.push_back(std::make_unique<CopyPropPass>());
  if (!std::getenv("NO_WBR"))
    pipeline.push_back(std::make_unique<WriteBarrierReductionPass>(fileView));
  if (!std::getenv("NO_DCE"))
    pipeline.push_back(std::make_unique<DCEPass>());
  if (!std::getenv("NO_EPROP"))
    pipeline.push_back(std::make_unique<PropEffectsPass>());
  if (!std::getenv("NO_RKEYCAST"))
    pipeline.push_back(std::make_unique<RemoveRedundantPropKeyCastPass>());
  if (!std::getenv("NO_REDKEYCAST"))
    pipeline.push_back(std::make_unique<ReduceComputedFieldOpsPass>());
}

void PassManager::addPass(std::unique_ptr<BBPass> pass) {
  pipeline.push_back(std::move(pass));
}

void PassManager::removePass(const std::string& name) {
  pipeline.erase(
    std::remove_if(pipeline.begin(), pipeline.end(),
      [&](const std::unique_ptr<BBPass>& p) { return p->name() == name; }),
    pipeline.end());
}

void PassManager::optimize(int level, std::set<double> taintedScopes) {
  if (std::getenv("PRINT_OPT_STAT")) printOptimizationStatus();

  for (auto& bbContView : fileView.bbContainerViews) {
    for (auto& ts : taintedScopes) {
      if (isScopeReachable(ts, bbContView.getScopeIdx(), iridiumBuildContext)) {
        bbContView.tainted = true;
      }
    }
    if (bbContView.tainted) continue;

    // Per-container fixed-point: repeat the pipeline until no pass reports a change
    bool anyChanged;
    do {
      anyChanged = false;
      for (auto& pass : pipeline) {
        bool changed = pass->run(bbContView, analysisManager);
        if (changed) {
          analysisManager.invalidate(bbContView);
          anyChanged = true;
        }
      }
    } while (anyChanged);
  }

  if (!std::getenv("NO_DEADBR"))
    doDeadBindingRemoval(fileView);

  // Mark tail calls
  for (auto& bbContView : fileView.bbContainerViews) {
    bbContView.cfgManager.traverseCFG(
      [&](Vertex v, std::shared_ptr<BBSEXP> bb) {
        for (auto& stmt : bb->args) {
          if (auto retStmt = std::dynamic_pointer_cast<ReturnSEXP>(stmt)) {
            if (auto tailCall = std::dynamic_pointer_cast<CallSiteSEXP>(retStmt->getObj()))
              tailCall->setTAILCALL();
          }
        }
      });
  }
}

void PassManager::justAnalysis(std::stringstream& ss, std::set<double> taintedScopes) {
  size_t envReadRemoteTotal = 0, envReadRemoteSafe = 0;
  size_t envWriteRemoteTotal = 0, envWriteRemoteSafe = 0;

  std::function<void(IRISEXP)> collectInfo = [&](IRISEXP currSEXP) {
    if (auto read = std::dynamic_pointer_cast<EnvReadSEXP>(currSEXP)) {
      if (read->hasSAFE() && read->hasFlag("MAP_INF")) {
        auto mapInf = read->getFlagString("MAP_INF");
        if (mapInf.find("js3$") == std::string::npos && mapInf != "NA")
          ss << mapInf;
      }
      if (auto o = std::dynamic_pointer_cast<RemoteEnvBindingSEXP>(read->getObj())) {
        envReadRemoteTotal++;
        if (read->hasSAFE()) envReadRemoteSafe++;
      }
      return;
    } else if (auto write = std::dynamic_pointer_cast<EnvWriteSEXP>(currSEXP)) {
      if (write->hasSAFE() && write->hasFlag("MAP_INF")) {
        auto mapInf = write->getFlagString("MAP_INF");
        if (mapInf.find("js3$") == std::string::npos && mapInf != "NA")
          ss << mapInf;
      }
      if (auto o = std::dynamic_pointer_cast<RemoteEnvBindingSEXP>(write->getLValTarget())) {
        envWriteRemoteTotal++;
        if (write->hasSAFE()) envWriteRemoteSafe++;
      }
    }
    for (auto& e : currSEXP->args) collectInfo(e);
  };

  for (auto& bbContView : fileView.bbContainerViews) {
    for (auto& ts : taintedScopes) {
      if (isScopeReachable(ts, bbContView.getScopeIdx(), iridiumBuildContext))
        bbContView.tainted = true;
    }
    if (bbContView.tainted) continue;

    auto& tdzaMap = analysisManager.get<TDZA>(bbContView);
    WriteBarrierReduction::currFileView = &fileView;
    for (auto v : boost::make_iterator_range(boost::vertices(bbContView.cfgManager.cfg)))
      WriteBarrierReduction::Transform(bbContView.cfgManager.cfg[v], tdzaMap.at(v));
    analysisManager.invalidate(bbContView);
  }

  for (auto& bbContView : fileView.bbContainerViews) {
    bbContView.cfgManager.traverseCFG(
      [&](Vertex v, std::shared_ptr<BBSEXP> bb) {
        for (auto& stmt : bb->args) collectInfo(stmt);
      });
  }

  std::cout << "SAFE: " << (envReadRemoteSafe + envWriteRemoteSafe)
            << " TOTAL: " << (envReadRemoteTotal + envWriteRemoteTotal) << std::endl;
}

std::shared_ptr<FileSEXP> PassManager::checkout() {
  return fileView.checkout();
}
