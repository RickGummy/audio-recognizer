#include "dsp/spectrogram.h"
#include "dsp/fft.h"

#include <cmath>

std::vector<Frame> compute_spectrogram(const std::vector<double> &samples,
                                       int sample_rate, int window_size,
                                       int hop_size) {
  std::vector<Frame> frames;

  const int N = samples.size();

  // slide window
  for (int start = 0; start + window_size <= N; start += hop_size) {
    // copy one chunk
    std::vector<double> chunk(window_size);
    for (int i = 0; i < window_size; i++) {
      double hann = 0.5 - 0.5 * std::cos(2 * M_PI * i / (window_size - 1));
      chunk[i] = samples[start + i] * hann;
    }

    Frame f;
    f.magnitudes = magnitude_spectrum(chunk);
    f.time_seconds = (double)start / sample_rate;
    frames.push_back(f);
  }

  return frames;
}
