#pragma once

#include "Iridium/Analysis/Domains/AnalysisManager.h"
#include "Iridium/OptimizationPasses/BBPass.h"

class PassManager
{
  FileView &fileView;
  std::unordered_map<int, IRIBUILDCONTEXT> iridiumBuildContext;

  std::vector<std::unique_ptr<BBPass>> pipeline;
  AnalysisManager AM;

  void buildPipeline(int level);
  bool isTainted(BBContainerView &bb, const std::set<double> &taintedScopes);

public:
  PassManager(FileView & fs, std::unordered_map<int, IRIBUILDCONTEXT> bc) : fileView(fs), iridiumBuildContext(bc) {}

  void justAnalysis(std::stringstream &, std::set<double>);
  void optimize(int level, std::set<double> taintedScopes);

  std::shared_ptr<FileSEXP> checkout();
};
