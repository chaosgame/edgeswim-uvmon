#include "profiler_wrapper.h"

#include <sstream>
#include <stack>

std::map<int, CpuProfilerWrapper::ExtraNodeData> CpuProfilerWrapper::BuildExtraNodeData(v8::CpuProfile *profile) {
  std::map<int, CpuProfilerWrapper::ExtraNodeData> extra_node_data;

  std::stack<std::pair<const v8::CpuProfileNode*, const v8::CpuProfileNode*> > state;
  state.push(std::make_pair(static_cast<const v8::CpuProfileNode*>(NULL), profile->GetTopDownRoot()));

  while (!state.empty()) {
    const v8::CpuProfileNode *parent = state.top().first;
    const v8::CpuProfileNode *node = state.top().second;
    state.pop();

    /* Because GetFunctionName and GetScriptResourceName internally allocate
     * memory on the GC heap, these functions are _not_ thread safe and must
     * be called from the main event thread.
     */
    ExtraNodeData &extra_data = extra_node_data[node->GetNodeId()];
    extra_data.function_name = std::string(*Nan::Utf8String(node->GetFunctionName()));
    extra_data.file_name = std::string(*Nan::Utf8String(node->GetScriptResourceName()));
    extra_data.parent = parent;
    extra_data.node = node;

    for (int i = 0; i < node->GetChildrenCount(); ++i) {
      state.push(std::make_pair(node, node->GetChild(i)));
    }
  }
  return extra_node_data;
}


std::map<int, int> CpuProfilerWrapper::BuildFilteredHits(
    v8::CpuProfile *profile,
    uint64_t profiling_ended_at_ms,
    uint64_t filter_from_ms) {
  std::map<int, int> filtered_hits;

  int samples_count = profile->GetSamplesCount();

  /* With the timestamps filter_from and profiling_ended_at, we can approximate
   * how many samples back we expect to look to find the first sample _after_
   * the timestamp filter_from.
   */
  int ms_since_last_sample = profiling_ended_at_ms - filter_from_ms;

  /* Since we sample every sampling_interval_ ms, we can use the ms_since_last_sample above and
   * divide it by the sampling interval to get the approximate number of samples back to start
   * checking from.
   */
  int i = std::max(samples_count - (int)(ms_since_last_sample / sampling_interval_ms_), 0);
  for (; i < samples_count; ++i) {

    /* GetEndTime() and GetSampleTimestamp() are both based off an arbitrary but
     * consistent point in time.  If we make the assumption that profiling_ended_at
     * is approximately at the same time as GetEndTime(), we can convert the sample's
     * timestamp (profiling_ended_at - sample_delta) and see if it occurred before
     * our filter_from timestamp.
     */
    uint64_t sample_delta_ms = (profile->GetEndTime() - profile->GetSampleTimestamp(i)) / 1000;

    /* If we started looking too early, continue without adding the sample hit to
     * filtered_hits.
     */
    if (profiling_ended_at_ms - sample_delta_ms <= filter_from_ms) continue;

    filtered_hits[profile->GetSample(i)->GetNodeId()]++;
  }

  return filtered_hits;
}


void CpuProfilerWrapper::LogFilteredProfile(
    v8::CpuProfile *profile,
    uint64_t profiling_ended_at,
    uint64_t filter_from) {
  if (profile == NULL || !is_profiling_) return;

  std::map<int, ExtraNodeData> extra_data = BuildExtraNodeData(profile);
  std::map<int, int> filtered_hits = BuildFilteredHits(profile, profiling_ended_at, filter_from);

  const int root_id = profile->GetTopDownRoot()->GetNodeId();

  /* Build the list of collapsed stacks for logging.
   *
   * https://github.com/brendangregg/FlameGraph/blob/master/stackcollapse.pl
   */
  std::stringstream stack;

  for (std::map<int, int>::iterator i = filtered_hits.begin(); i != filtered_hits.end(); ++i) {
    int node_id = i->first;
    int hits = i->second;

    for (int i = node_id; i != root_id; i = extra_data[i].parent->GetNodeId()) {
      if (i != node_id) stack << ";";

      if (extra_data[i].function_name.length()) {
        stack << extra_data[i].function_name;
      } else {
        stack << "[unknown]";
      }

      stack << "(";

      if (extra_data[i].file_name.length()) {
        stack << extra_data[i].file_name;
      } else {
        stack << "?";
      }

      stack << ":";

      if (extra_data[i].node->GetLineNumber() != v8::CpuProfileNode::kNoLineNumberInfo) {
        stack << extra_data[i].node->GetLineNumber();
      } else {
        stack << "?";
      }

      stack << ")";
    }

    stack << " ";
    stack << hits * sampling_interval_ms_;
    stack << "\n";
  }

  uv_fs_t *write_req = new uv_fs_t;

  uv_buf_t bufs[1];
  bufs[0].len = stack.str().size();
  bufs[0].base = new char[bufs[0].len];
  std::memcpy(bufs[0].base, stack.str().data(), bufs[0].len);

  uv_fs_write(
    uv_default_loop(),
    write_req,
    /* uv_file */ log_file_,
    bufs,
    /* nbufs= */ 1,
    /* offset= */ -1,
    CpuProfilerWrapper::LogWrittenCallback);
}

void CpuProfilerWrapper::LogWrittenCallback(uv_fs_t *write_req) {
  delete[] write_req->bufs[0].base;
  uv_fs_req_cleanup(write_req);
  delete write_req;
}

void CpuProfilerWrapper::Start(char *filename) {
  if (is_profiling_) {
    return;
  }

  profiler_ = v8::Isolate::GetCurrent()->GetCpuProfiler();
  profiler_->SetSamplingInterval(1000 * sampling_interval_ms_);

  uv_fs_t open_req;
  log_file_ = uv_fs_open(
    uv_default_loop(),
    &open_req,
    filename,
    O_WRONLY|O_APPEND|O_CREAT,
    0644,
    NULL);
  uv_fs_req_cleanup(&open_req);

  v8::Local<v8::String> title = Nan::New<v8::String>("white").ToLocalChecked();
  profiler_->StartProfiling(title, /* record_samples= */ true);
  profiling_started_at_ms_ = uv_now(uv_default_loop());
  is_profiling_ = true;
}


void CpuProfilerWrapper::Stop() {
  if (!is_profiling_) {
    return;
  }

  v8::Local<v8::String> title;
  if (color_ == Black) {
    title = Nan::New<v8::String>("black").ToLocalChecked();
  } else {
    title = Nan::New<v8::String>("white").ToLocalChecked();
  }

  uv_fs_t close_req;
  uv_fs_close(
    uv_default_loop(),
    &close_req,
    log_file_,
    NULL);
  uv_fs_req_cleanup(&close_req);

  profiler_->StopProfiling(title);
  is_profiling_ = true;
}


v8::CpuProfile *CpuProfilerWrapper::Sample() {
  if (!is_profiling_) {
    return NULL;
  }

  v8::Local<v8::String> old_title;
  v8::Local<v8::String> new_title;
  if (color_ == Black) {
    old_title = Nan::New<v8::String>("black").ToLocalChecked();
    new_title = Nan::New<v8::String>("white").ToLocalChecked();
    color_ = White;
  } else {
    old_title = Nan::New<v8::String>("white").ToLocalChecked();
    new_title = Nan::New<v8::String>("black").ToLocalChecked();
    color_ = Black;
  }

  /* We're starting the new profile before we stop the old profile because V8
   * will optimistically clean up it's profiling handlers whenever there aren't
   * any active profilers on the sytem.  This ensures that we don't do that
   * work unnecessarily.
   *
   * This isn't just a premature optimization -- in node 0.12, cleaning up includes
   * synchronously joining with the sampling thread which ticks sampling_interval_ms_
   * (default 10ms) -- If we stopped the old profiler before the new profiler was
   * started, we would on average add 5ms of unnecessary synchronous waiting on the
   * main thread.  This only gets worse with larger sampling intervals.
   */
  profiler_->StartProfiling(new_title, /* record_samples= */ true);
  profiling_started_at_ms_ = uv_now(uv_default_loop());

  return profiler_->StopProfiling(old_title);
}
