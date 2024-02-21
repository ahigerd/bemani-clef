/*
 * WMA decoder copyright (c) 2020 Adam Higerd
 * FFmpeg copyright (c) 2000-2004 The FFmpeg Project, Fabrice Bellard,
 *     Michael Niedermayer <michaelni@gmx.at>
 *
 * bemani-clef is free software. Most of its source code is licensed
 * under the MIT license, but files with this notice are derived from
 * FFmpeg. As a result, these files and any binary distribution compiled
 * from them may be redistributed and/or modified under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to:
 *   Free Software Foundation, Inc.
 *   51 Franklin Street, Fifth Floor
 *   Boston MA  02110-1301
 *   USA
 */

struct LSP {
  static LSP* get(int frameLen) {
    if (!cache.count(frameLen)) {
      cache.emplace(frameLen, frameLen);
    }
    return &cache.at(frameLen);
  }

  LSP(int frameLen) {
    for (int i = 0; i < frameLen; i++) {
      lspCos.push_back(std::cos(M_PI * i / frameLen) * 2);
    }

    if (!lspPowE.size()) {
      lspPowE.reserve(256);
      for (int i = 0; i < 256; i++) {
        lspPowE.push_back(std::exp2f((i - 126) * -0.25));
      }

      float b = 1.0;
      lspPowM1.reserve(128);
      lspPowM2.reserve(128);
      for (int i = 0; i < 128; i++) {
        float m = 128 + i;
        float a = 1 / std::sqrt(std::sqrt(m * 64));
        lspPowM1.push_back(2 * a - b);
        lspPowM2.push_back(b - a);
        b = a;
      }
    }
  }

  struct Curve {
    float exponents[10];
    float maxExponent;
  };

  Curve toCurve(int blockLen, float coefs[10]) const {
    Curve rv;
    rv.maxExponent = 0;
    for (int i = 0; i < blockLen; i++) {
      float p = 0.5f, q = 0.5f, w = lspCos[i];
      for (int j = 1; j < 10; j += 2) {
        q *= w - coefs[j - 1];
        p *= w - coefs[j];
      }
      p *= p * (2 - w);
      q *= q * (2 + w);
      float v = rv.exponents[i] = powM14(p + q);
      if (v > rv.maxExponent) {
        rv.maxExponent = v;
      }
    }
    return rv;
  }

  inline float powM14(float x) const {
    uint32_t u;
    std::memcpy(&u, &x, sizeof(float));
    float e = lspPowE[u >> 23];
    uint32_t m = (u >> 13) & ((1 << 10) - 1);
    uint32_t t = ((u << 10) & ((1 << 23) - 1)) | (127 << 23);
    float tf;
    std::memcpy(&tf, &t, sizeof(float));
    return e * (lspPowM1[m] + lspPowM2[m] * tf);
  }

  std::vector<float> lspCos;
  static std::vector<float> lspPowE, lspPowM1, lspPowM2;

private:
  static std::unordered_map<int, LSP> cache;
};
std::unordered_map<int, LSP> LSP::cache;
std::vector<float> LSP::lspPowE, LSP::lspPowM1, LSP::lspPowM2;
