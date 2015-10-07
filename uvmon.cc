
#include <nan.h>
#include <node.h>
#include <v8.h>
#include <v8-profiler.h>
#include <uv.h>

#include <cstring>
#include <iostream>

#include "histogram.h"
#include "profiler_wrapper.h"

using namespace v8;

static uv_check_t check_handle;

static uint64_t last_ticked_at_ms;

static BucketingHistogram loop_stats(0, 1000, 10);

static CpuProfilerWrapper profiler_wrapper(10);

/**
 * This is the callback that we register to be called at the end of
 * the uv_run() loop.
 */
static void check_run(uv_check_t* handle) {
  uv_update_time(uv_default_loop());
  uint64_t now_ms = uv_now(uv_default_loop());
  uint64_t tick_delta_ms = now_ms - last_ticked_at_ms;

  uint64_t profiling_delta_ms = profiler_wrapper.GetCurrentProfilingDuration();
  if (profiler_wrapper.IsProfiling() && (profiling_delta_ms > 10000 || tick_delta_ms > 100)) {
    v8::CpuProfile *profile = profiler_wrapper.Sample();

    if (tick_delta_ms > 100) {
      profiler_wrapper.LogFilteredProfile(profile, now_ms, last_ticked_at_ms);
    }

    profile->Delete();
  }

  loop_stats.Sample(tick_delta_ms);
  last_ticked_at_ms = now_ms;
}

/**
 * Returns the data gathered from the uv_run() loop.
 * Resets all counters when called.
 */
NAN_METHOD(getData) {
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();

  Nan::Set(obj, Nan::New<v8::String>("count").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetSampleCount()));
  Nan::Set(obj, Nan::New<v8::String>("p50_ms").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetApproximatePercentile(50)));
  Nan::Set(obj, Nan::New<v8::String>("p95_ms").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetApproximatePercentile(95)));
  Nan::Set(obj, Nan::New<v8::String>("p99_ms").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetApproximatePercentile(99)));
  Nan::Set(obj, Nan::New<v8::String>("avg_ms").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetAverage()));
  Nan::Set(obj, Nan::New<v8::String>("max_ms").ToLocalChecked(),
      Nan::New<v8::Number>(loop_stats.GetMaximum()));

  loop_stats.Reset();
  info.GetReturnValue().Set(obj);
}

/**
 */
NAN_METHOD(stopProfiling) {
  profiler_wrapper.Stop();
  info.GetReturnValue().SetUndefined();
}

/**
 */
NAN_METHOD(startProfiling) {
  if (info.Length() != 1 || !info[0]->IsString()) {
    return Nan::ThrowError("Expected startProfiling(filename)");
  }

  profiler_wrapper.Start(*Nan::Utf8String(info[0]));
  info.GetReturnValue().SetUndefined();
}

/**
 * Initialization and registration of methods with node.
 */
void init(v8::Handle<v8::Object> target) {
  last_ticked_at_ms = uv_now(uv_default_loop());

  /* set up uv_run callback */
  uv_check_init(uv_default_loop(), &check_handle);
  uv_unref(reinterpret_cast<uv_handle_t*>(&check_handle));
  uv_check_start(&check_handle, check_run);

  Nan::SetMethod(target, "getData", getData);
  Nan::SetMethod(target, "startProfiling", startProfiling);
  Nan::SetMethod(target, "stopProfiling", stopProfiling);
}

NODE_MODULE(uvmon, init);
