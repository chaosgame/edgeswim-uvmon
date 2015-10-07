
#pragma once

#include <cstddef>
#include <cmath>
#include <vector>

class LogarithmicBucketingHistogram {
private:
  double sample_min_;
  double sample_max_;
  double sample_sum_;
  size_t sample_count_;

  std::vector<std::pair<double, size_t> > vector_;

public:
  /**
   */
  LogarithmicBucketingHistogram(int num_buckets);

  /**
   */
  void Reset();

  /**
   */
  void Sample(double value);

  /**
   */
  double GetApproximatePercentile(size_t percentile);

  /**
   */
  double GetAverage() {
    if (sample_count_) {
      return sample_sum_ / sample_count_;
    } else {
      return NAN;
    }
  }

  /**
   */
  double GetMaximum() {
    return sample_max_;
  }

  /**
   */
  double GetMinimum() {
    return sample_min_;
  }

  /**
   */
  size_t GetSampleCount() {
    return sample_count_;
  }
};
