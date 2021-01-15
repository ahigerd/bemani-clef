#ifndef B2W_WMA_MDCT_H
#define B2W_WMA_MDCT_H

#include <cmath>
#include <cstdint>
#include <vector>

struct MDCT {
  static MDCT* get(int numBits);

  MDCT(int numBits);

  void calcInverse(float* coefs, std::vector<float>& output) const;

  int numBits, mdctSize;

private:
  void inverse_mdct(float *buffer) const;
  std::vector<uint16_t> bitReverse;
  std::vector<float> A, B, C;
};

#endif
