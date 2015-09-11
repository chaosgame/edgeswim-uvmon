
#pragma once

#include <cmath>
#include <vector>

class BucketingHistogram {
private:
  const double bucket_min_;
  const double bucket_max_;
  const double bucket_step_;

  double sample_min_;
  double sample_max_;
  double sample_sum_;
  size_t sample_count_;

  std::vector<std::pair<double, size_t> > vector_;

public:
  /**
   */
  BucketingHistogram(double min, double max, double step);

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
