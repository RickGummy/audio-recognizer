#pragma once
#include <vector>

struct Frame {
  std::vector<double> magnitudes;
  double time_seconds;
};

std::vector<Frame> compute_spectrogram(const std::vector<double> &samples,
                                       int sample_rate, int window_size,
                                       int hop_size);
