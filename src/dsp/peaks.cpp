#include "dsp/peaks.h"

std::vector<Peak> extract_peaks(const std::vector<Frame> &spectrogram,
                                int neighborhood_size, double threshold) {
  std::vector<Peak> peaks;

  const int frames = spectrogram.size();

  for (int i = 0; i < frames; i++) {
    const std::vector<double> &mags = spectrogram[i].magnitudes;

    const int bins = mags.size();
    for (int j = 0; j < bins; j++) {
      double value = mags[j];

      if (value < threshold) {
        continue;
      }

      bool is_max = true;
      for (int di = -neighborhood_size; di <= neighborhood_size && is_max;
           di++) {
        for (int dj = -neighborhood_size; dj <= neighborhood_size; dj++) {
          int new_i = i + di;
          int new_j = j + dj;

          if (new_i < 0 || new_i >= frames) {
            continue;
          }
          if (new_j < 0 || new_j >= spectrogram[new_i].magnitudes.size()) {
            continue;
          }

          if (di == 0 && dj == 0)
            continue;

          if (spectrogram[new_i].magnitudes[new_j] > value) {
            is_max = false;
            break;
          }
        }
      }

      if (is_max) {
        peaks.push_back(Peak{i, j, value});
      }
    }
  }

  return peaks;
}
