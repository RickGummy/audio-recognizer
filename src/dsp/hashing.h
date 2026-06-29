#pragma once

#include "dsp/peaks.h"
#include <cstdint>
#include <vector>

struct Fingerprint {
  uint32_t hash;
  int anchor_time;
};

std::vector<Fingerprint> generate_hashes(const std::vector<Peak> &peaks,
                                         int fan_out = 5,
                                         int max_time_delta = 100);
