#include "dsp/fft.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

// Helper: index of large value in vector
static int maxarg(const std::vector<double> &v) {
  int max = 0;
  for (int i = 1; i < v.size(); i++) {
    if (v[i] > v[max]) {
      max = i;
    }
  }

  return max;
}

// Test 1: pure 440 Hz sin wave
static void test_440() {
  const int sample_rate = 44100;
  const int N = 4096;
  const double frequency = 440.0;

  std::vector<double> samples(N);
  for (int i = 0; i < N; i++) {
    samples[i] = std::sin(2 * M_PI * frequency * i / sample_rate);
  }
  std::vector<double> ms = magnitude_spectrum(samples);

  int peak = maxarg(ms);

  double peak_hz = (double)peak * sample_rate / N;

  std::cout << "peak = " << peak << "-> " << peak_hz << std::endl;

  assert(std::abs(peak_hz - frequency) < 15.0);
}

static void test_signal() {
  std::vector<double> samples(8, 1.0);
  std::vector<double> mag = magnitude_spectrum(samples);
  assert(maxarg(mag) == 0);
}

int main() {
  test_440();
  test_signal();
  std::cout << "test_fft: passed" << std::endl;
  return 0;
}
