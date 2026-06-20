#pragma once
#include "dsp/spectrogram.h"
#include <vector>

struct Peak {
  int time_frame;
  int freq_bin;
  double magnitude;
};

std::vector<Peak> extract_peaks(const std::vector<Frame> &spectrogram,
                                int neighborhood_size = 10,
                                double threshold = 10.0);
