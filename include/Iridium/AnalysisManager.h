#pragma once
#include "Iridium/Analysis/IntraProceduralAnalysis.h"
#include <any>
#include <cassert>
#include <typeindex>
#include <unordered_map>

class AnalysisManager {
  struct CacheKey {
    std::type_index type;
    BBContainerView* container;

    bool operator==(const CacheKey& o) const {
      return type == o.type && container == o.container;
    }
  };

  struct CacheKeyHash {
    size_t operator()(const CacheKey& k) const {
      return std::hash<std::type_index>()(k.type) ^ (std::hash<void*>()(k.container) << 1);
    }
  };

  struct CacheEntry {
    std::any result;
    bool stale = true;
  };

  std::unordered_map<std::type_index, std::unique_ptr<AnalysisBase>> registry;
  std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> cache;

public:
  template <typename Domain>
  void registerAnalysis(std::unique_ptr<IntraProceduralAnalysis<Domain>> analysis) {
    registry[std::type_index(typeid(Domain))] = std::move(analysis);
  }

  template <typename Domain>
  const std::unordered_map<Vertex, Domain>& get(BBContainerView& view) {
    CacheKey key{std::type_index(typeid(Domain)), &view};
    auto& entry = cache[key];
    if (entry.stale) {
      auto it = registry.find(std::type_index(typeid(Domain)));
      assert(it != registry.end() && "Analysis not registered with AnalysisManager");
      auto* analysis = static_cast<IntraProceduralAnalysis<Domain>*>(it->second.get());
      entry.result = analysis->compute(view);
      entry.stale = false;
    }
    return std::any_cast<const std::unordered_map<Vertex, Domain>&>(entry.result);
  }

  void invalidate(BBContainerView& view) {
    for (auto& [key, entry] : cache) {
      if (key.container == &view)
        entry.stale = true;
    }
  }
};
