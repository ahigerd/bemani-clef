#ifndef B2W_WMA_MDCT_H
#define B2W_WMA_MDCT_H

#include <cmath>
#include <cstdint>
#include <vector>
#include "v_mdct.h"

struct MDCT {
  static MDCT* get(int numBits);

  MDCT(int numBits);

  void calcInverse(float* coefs, std::vector<float>& output) const;

  int numBits, mdctSize;

private:
  mdct_lookup v;
};

#endif
