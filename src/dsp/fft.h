#pragma once

#include <complex>
#include <vector>

// the fast fourier transform from time to frequency
void fft(std::vector<std::complex<double>> &x);

// translates to complex
std::vector<double> magnitude_spectrum(const std::vector<double> &samples);
