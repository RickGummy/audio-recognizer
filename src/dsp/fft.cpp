#include "fft.h"

#include <cmath>

void fft(std::vector<std::complex<double>> &x) {
  // do the cooley tukey fft
  const size_t N = x.size();
  if (N <= 1) {
    return;
  }

  // divide
  std::vector<std::complex<double>> even(N / 2);
  std::vector<std::complex<double>> odd(N / 2);
  for (size_t i = 0; i < N / 2; i++) {
    even[i] = x[2 * i];
    odd[i] = x[2 * i + 1];
  }

  // recursion
  fft(even);
  fft(odd);

  for (size_t k = 0; k < N / 2; k++) {
    std::complex<double> w = std::polar(1.0, -2.0 * M_PI * k / N) * odd[k];
    x[k] = even[k] + w;
    x[k + N / 2] = even[k] - w;
  }
}

size_t next_pow2(size_t n) {
  size_t p = 1;

  while (p < n) {
    p <<= 1;
  }

  return p;
}

std::vector<double> magnitude_spectrum(const std::vector<double> &samples) {
  const size_t N = next_pow2(samples.size());

  std::vector<std::complex<double>> x(N);
  for (size_t i = 0; i < samples.size(); i++) {
    x[i] = std::complex<double>(samples[i], 0.0);
  }

  fft(x);

  std::vector<double> mag(N / 2);
  for (size_t k = 0; k < N / 2; k++) {
    mag[k] = std::abs(x[k]);
  }

  return mag;
}
