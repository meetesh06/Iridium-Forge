#pragma once
#include "Iridium/Structure/CFGManager.h"
#include <unordered_map>

class BBContainerView;

class AnalysisBase {
public:
  virtual ~AnalysisBase() = default;
};

template <typename Domain>
class IntraProceduralAnalysis : public AnalysisBase {
public:
  virtual ~IntraProceduralAnalysis() = default;
  virtual std::unordered_map<Vertex, Domain> compute(BBContainerView& view) = 0;
};
