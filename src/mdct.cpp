#include "mdct.h"
#include <unordered_map>

MDCT* MDCT::get(int numBits) {
  static std::unordered_map<int, MDCT> cache;
  if (!cache.count(numBits)) {
    cache.emplace(numBits, numBits);
  }
  return &cache.at(numBits);
}

MDCT::MDCT(int numBits) : numBits(numBits), mdctSize(1 << numBits) {
  mdct_init(&v, 1 << numBits);
}

void MDCT::calcInverse(float* coefs, std::vector<float>& output) const
{
  output.resize(mdctSize * 2);
  mdct_backward(&v, coefs, output.data());
  for (int i = 0; i < mdctSize * 2; i++) {
    output[i] /= -32768.0;
  }
}
