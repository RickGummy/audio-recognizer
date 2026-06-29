#include "dsp/hashing.h"
#include "dsp/peaks.h"

#include <cassert>
#include <iostream>
#include <vector>

static void test_basic_pair() {
  std::vector<Peak> peaks = {
      {0, 40, 100},
      {5, 60, 100},
  };

  auto fps = generate_hashes(peaks, 5, 100);

  assert(fps.size() == 1);
  assert(fps[0].anchor_time == 0);
  std::cout << "basic pair good" << std::endl;
}
