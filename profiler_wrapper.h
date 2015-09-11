
#pragma once

#include <string>
#include <map>

#include <nan.h>
#include <v8.h>
#include <v8-profiler.h>
#include <uv.h>

class CpuProfilerWrapper {
private:
  struct ExtraNodeData {
    std::string function_name;
    std::string file_name;

    const v8::CpuProfileNode *parent;
    const v8::CpuProfileNode *node;
  };

  enum Color { Black = 0, White = 1 };

  Color color_;
  uint64_t sampling_interval_ms_;
  uint64_t profiling_started_at_ms_;
  bool is_profiling_;

  uv_file log_file_;

  v8::CpuProfiler *profiler_;

  static void LogWrittenCallback(uv_fs_t *write_req);

  std::map<int, ExtraNodeData> BuildExtraNodeData(v8::CpuProfile *profile);
  std::map<int, int> BuildFilteredHits(v8::CpuProfile *, uint64_t, uint64_t);

public:
  /**
   */
  CpuProfilerWrapper(uint64_t sampling_interval_ms)
    : color_(CpuProfilerWrapper::White)
    , sampling_interval_ms_(sampling_interval_ms)
    , is_profiling_(false)
    , log_file_(-1)
    , profiler_(NULL) {}

  /**
   */
  void Start(char *filename);

  /**
   */
  void Stop();

  /**
   */
  v8::CpuProfile *Sample();

  /**
   */
  bool IsProfiling () {
    return is_profiling_;
  }

  /**
   */
  uint64_t GetCurrentProfilingDuration() {
    return uv_now(uv_default_loop()) - profiling_started_at_ms_;
  }

  /**
   */
  void LogFilteredProfile(v8::CpuProfile *, uint64_t, uint64_t);
};

