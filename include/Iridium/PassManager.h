#pragma once
#include "Iridium/BBPass.h"
#include "Iridium/AnalysisManager.h"
#include "Iridium/Globals.h"
#include "Iridium/Structure/FileView.h"
#include "generated/IridiumTypes.h"
#include <memory>
#include <sstream>
#include <vector>

class PassManager {
public:
  PassManager(FileView& fs, std::unordered_map<int, IRIBUILDCONTEXT> bc);

  void addPass(std::unique_ptr<BBPass> pass);
  void removePass(const std::string& name);

  void justAnalysis(std::stringstream&, std::set<double>);
  void optimize(int level, std::set<double> taintedScopes);

  std::shared_ptr<FileSEXP> checkout();

private:
  FileView& fileView;
  std::unordered_map<int, IRIBUILDCONTEXT> iridiumBuildContext;
  AnalysisManager analysisManager;
  std::vector<std::unique_ptr<BBPass>> pipeline;
};
