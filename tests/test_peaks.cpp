#include "dsp/peaks.h"
#include "dsp/spectrogram.h"

#include <cassert>
#include <iostream>
#include <vector>

// build a blank spectrogram
static std::vector<Frame> blank(int frames, int bins) {
  std::vector<Frame> spec(frames);

  for (int t = 0; t < frames; t++) {
    spec[t].magnitudes = std::vector<double>(bins, 0.0);
    spec[t].time_seconds = t * 0.01;
  }

  return spec;
}

// one bright spot, should return that frame
static void test_one_peak() {
  auto spec = blank(5, 100);
  spec[2].magnitudes[50] = 100.0;

  auto peaks = extract_peaks(spec, 10, 10.0);

  assert(peaks.size() == 1);
  assert(peaks[0].time_frame == 2);
  assert(peaks[0].freq_bin == 50);
  std::cout << "one-peak ok" << std::endl;
}

// the speak is below a peak
static void test_threshold() {
  auto spec = blank(5, 100);
  spec[2].magnitudes[50] = 5.0;

  auto peaks = extract_peaks(spec, 10, 10.0);

  assert(peaks.empty());

  std::cout << "good" << std::endl;
}

// two peaks
static void test_neighborhood() {
  auto spec = blank(5, 100);
  spec[2].magnitudes[50] = 100.0;
  spec[2].magnitudes[55] = 50.0;

  auto peaks = extract_peaks(spec, 10, 10.0);

  assert(peaks.size() == 1);
  assert(peaks[0].freq_bin == 50);
  std::cout << "good" << std::endl;
}

int main() {
  test_one_peak();
  test_threshold();
  test_neighborhood();

  std::cout << "all passed" << std::endl;
  return 0;
}
