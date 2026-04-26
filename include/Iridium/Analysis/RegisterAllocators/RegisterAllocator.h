#pragma once
#include <cstddef>

struct RAContext;

// Abstract interface for register allocation strategies.
// Each concrete allocator receives all IR context through run() and drives itself.
// To swap allocators, change the concrete type at the call site.
class RegisterAllocator
{
public:
  virtual ~RegisterAllocator() = default;

  virtual void run(const RAContext &ctx) = 0;

  virtual size_t getMaxIdx() const = 0;
  virtual void setMaxIdx(size_t idx) = 0;
};
