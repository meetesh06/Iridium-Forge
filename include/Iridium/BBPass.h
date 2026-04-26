#pragma once
#include <string>

class BBContainerView;
class AnalysisManager;

class BBPass {
public:
  virtual ~BBPass() = default;
  virtual bool run(BBContainerView& view, AnalysisManager& am) = 0;
  virtual std::string name() const = 0;
};
