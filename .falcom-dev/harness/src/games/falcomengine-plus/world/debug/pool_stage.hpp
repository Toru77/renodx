#pragma once

// Which world-pool stage the current thread is in, for crash reports
// (debug/crash_log.hpp). Costs one thread-local pointer store per stage
// change; nothing is logged from here.

namespace falcom_world {

inline thread_local const char* t_pool_stage = nullptr;

inline const char* CurrentPoolStage() { return t_pool_stage; }

// Sets the stage for a scope and restores the previous one on exit.
class PoolStageScope {
 public:
  explicit PoolStageScope(const char* stage) : previous_(t_pool_stage) { t_pool_stage = stage; }
  ~PoolStageScope() { t_pool_stage = previous_; }
  PoolStageScope(const PoolStageScope&) = delete;
  PoolStageScope& operator=(const PoolStageScope&) = delete;

  // Moves to the next stage inside the same scope.
  void Set(const char* stage) { t_pool_stage = stage; }

 private:
  const char* previous_;
};

}  // namespace falcom_world
