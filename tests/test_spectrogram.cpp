#include "dsp/spectrogram.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static int maxarg(const std::vector<double> &v) {
  int max = 0;
  for (int i = 1; i < v.size(); i++) {
    if (v[i] > v[max]) {
      max = i;
    }
  }

  return max;
}

// checks if the number of frames is correct
// frames == (samples - window) / hop + 1
static void test_frame_count() {
  const int sample_rate = 44100;
  const int window_size = 4096;
  const int hop_size = 512;
  const int N = 44100;

  std::vector<double> samples(N, 0.0);

  auto spec = compute_spectrogram(samples, sample_rate, window_size, hop_size);

  int expected = (N - window_size) / hop_size + 1;
  std::cout << "frames = " << spec.size() << std::endl;
  std::cout << "expected = " << expected << "\n";

  assert(spec.size() == expected);
}

// checks a sine peaks at the correct bin
static void test_sine_peaks() {
  const int sample_rate = 44100;
  const int window_size = 4096;
  const int hop_size = 512;
  const int N = 44100;
  const double frequency = 440.0;

  std::vector<double> samples(N);
  for (int i = 0; i < N; i++) {
    samples[i] = std::sin(2 * M_PI * frequency * i / sample_rate);
  }

  auto spec = compute_spectrogram(samples, sample_rate, window_size, hop_size);
  assert(!spec.empty());

  for (const auto &s : spec) {
    int peak = maxarg(s.magnitudes);
    double peak_hz = (double)peak * sample_rate / window_size;

    assert(std::abs(peak_hz - frequency) < 15.0);
  }
  std::cout << "all " << spec.size() << " frames peak at 440HZ\n";
}

// time axis advances each frame
static void test_time_axis() {
  const int sample_rate = 44100;
  const int window_size = 4096;
  const int hop_size = 512;
  const int N = 44100;

  std::vector<double> samples(N, 0.0);
  auto spec = cmopute_spectrogram(samples, sample_rate, window_size, hop_size);

  assert(spec.size() >= 2);

  double dt = (double)hop_size / sample_rate;
  assert(std::abs(spec[0].time_seconds - 0.0) < 1e-9);
  assert(std::abs(spec[1].time_seconds - dt) < 1e-9);

  std::cout << "time axis ok, dt = " << dt << std::endl;
}

int main() {
  test_frame_count();
  test_sine_peaks();
  test_time_axis();
  std::cout << "all spectrofram tests passed" << std::endl;
  return 0;
}
