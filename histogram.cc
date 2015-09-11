
#include "histogram.h"

#include <vector>

BucketingHistogram::BucketingHistogram(double min, double max, double step)
  : bucket_min_(min)
  , bucket_max_(max)
  , bucket_step_(step)
  , sample_sum_(0.0)
  , sample_count_(0)
  , vector_(std::ceil((max - min) / step) + 2) {
}

void BucketingHistogram::Reset() {
  sample_count_ = 0;
  sample_sum_ = 0.0;
  sample_min_ = 0.0;
  sample_max_ = 0.0;

  std::fill(vector_.begin(), vector_.end(), std::make_pair(0.0, 0));
}

void BucketingHistogram::Sample(double value) {
  size_t bucket;

  if (value > bucket_max_) {
    bucket = vector_.size() - 2;
  } else if (value < bucket_min_) {
    bucket = vector_.size() - 1;
  } else {
    bucket = (size_t)((value - bucket_min_) / bucket_step_);
  }

  sample_sum_ += value;
  sample_count_++;

  if (sample_max_ < value || sample_count_ == 1) {
    sample_max_ = value;
  }

  if (sample_min_ > value || sample_count_ == 1) {
    sample_min_ = value;
  }

  vector_[bucket].first += value;
  vector_[bucket].second++;
}

double BucketingHistogram::GetApproximatePercentile(size_t percentile) {
  size_t necessary_samples = std::ceil(percentile / 100.0 * sample_count_);
  size_t samples = 0;

  for (size_t i = 0; i < vector_.size(); ++i) {
    samples += vector_[i].second;
    if (samples >= necessary_samples) {
      return vector_[i].first / vector_[i].second;
    }
  }

  /* We will only ever reach this point if percentile is > 100 */
  return NAN;
}
