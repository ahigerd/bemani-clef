// With many thanks to Sean Barrett for dedicating his Vorbis decoder to the public domain.
// http://nothings.org/stb_vorbis/

#include "mdct.h"
#include <unordered_map>
#include <cstring>
#include <iostream>

void MDCT::inverse_mdct(float *buffer) const
{
  int n = mdctSize;
  float s;
  int i,k,k2,k4, n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l;
  int n3_4 = n - n4, ld;
  // how can they claim this only uses N words?!
  // oh, because they're only used sparsely, whoops
  float u[1 << 13], X[1 << 13], v[1 << 13], w[1 << 13];
  // set up twiddle factors
  const float* A = this->A.data();
  const float* B = this->B.data();
  const float* C = this->C.data();


  // IMDCT algorithm from "The use of multirate filter banks for coding of high quality digital audio"
  // Note there are bugs in that pseudocode, presumably due to them attempting
  // to rename the arrays nicely rather than representing the way their actual
  // implementation bounces buffers back and forth. As a result, even in the
  // "some formulars corrected" version, a direct implementation fails. These
  // are noted below as "paper bug".

  // copy and reflect spectral data
  for (k=0; k < n2; ++k) u[k] = buffer[k];
  for (   ; k < n ; ++k) u[k] = -buffer[n - k - 1];
  // kernel from paper
  // step 1
  for (k=k2=k4=0; k < n4; k+=1, k2+=2, k4+=4) {
    v[n-k4-1] = (u[k4] - u[n-k4-1]) * A[k2]   - (u[k4+2] - u[n-k4-3])*A[k2+1];
    v[n-k4-3] = (u[k4] - u[n-k4-1]) * A[k2+1] + (u[k4+2] - u[n-k4-3])*A[k2];
  }
  // step 2
  for (k=k4=0; k < n8; k+=1, k4+=4) {
    w[n2+3+k4] = v[n2+3+k4] + v[k4+3];
    w[n2+1+k4] = v[n2+1+k4] + v[k4+1];
    w[k4+3]    = (v[n2+3+k4] - v[k4+3])*A[n2-4-k4] - (v[n2+1+k4]-v[k4+1])*A[n2-3-k4];
    w[k4+1]    = (v[n2+1+k4] - v[k4+1])*A[n2-4-k4] + (v[n2+3+k4]-v[k4+3])*A[n2-3-k4];
  }
  // step 3
  ld = numBits;
  for (l=0; l < ld-3; ++l) {
    int k0 = n >> (l+2), k1 = 1 << (l+3);
    int rlim = n >> (l+4), r4, r;
    int s2lim = 1 << (l+2), s2;
    for (r=r4=0; r < rlim; r4+=4,++r) {
      for (s2=0; s2 < s2lim; s2+=2) {
        u[n-1-k0*s2-r4] = w[n-1-k0*s2-r4] + w[n-1-k0*(s2+1)-r4];
        u[n-3-k0*s2-r4] = w[n-3-k0*s2-r4] + w[n-3-k0*(s2+1)-r4];
        u[n-1-k0*(s2+1)-r4] = (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1]
          - (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1+1];
        u[n-3-k0*(s2+1)-r4] = (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1]
          + (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1+1];
      }
    }
    if (l+1 < ld-3) {
      // paper bug: ping-ponging of u&w here is omitted
      memcpy(w, u, sizeof(u));
    }
  }

  // step 4
  for (i=0; i < n8; ++i) {
    int j = bitReverse.at(i);
    //assert(j < n8);
    if (i == j) {
      // paper bug: original code probably swapped in place; if copying,
      //            need to directly copy in this case
      int i8 = i << 3;
      v[i8+1] = u[i8+1];
      v[i8+3] = u[i8+3];
      v[i8+5] = u[i8+5];
      v[i8+7] = u[i8+7];
    } else if (i < j) {
      int i8 = i << 3, j8 = j << 3;
      v[j8+1] = u[i8+1], v[i8+1] = u[j8 + 1];
      v[j8+3] = u[i8+3], v[i8+3] = u[j8 + 3];
      v[j8+5] = u[i8+5], v[i8+5] = u[j8 + 5];
      v[j8+7] = u[i8+7], v[i8+7] = u[j8 + 7];
    }
  }
  // step 5
  for (k=0; k < n2; ++k) {
    w[k] = v[k*2+1];
  }
  // step 6
  for (k=k2=k4=0; k < n8; ++k, k2 += 2, k4 += 4) {
    u[n-1-k2] = w[k4];
    u[n-2-k2] = w[k4+1];
    u[n3_4 - 1 - k2] = w[k4+2];
    u[n3_4 - 2 - k2] = w[k4+3];
  }
  // step 7
  for (k=k2=0; k < n8; ++k, k2 += 2) {
    v[n2 + k2 ] = ( u[n2 + k2] + u[n-2-k2] + C[k2+1]*(u[n2+k2]-u[n-2-k2]) + C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2;
    v[n-2 - k2] = ( u[n2 + k2] + u[n-2-k2] - C[k2+1]*(u[n2+k2]-u[n-2-k2]) - C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2;
    v[n2+1+ k2] = ( u[n2+1+k2] - u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2;
    v[n-1 - k2] = (-u[n2+1+k2] + u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2;
  }
  // step 8
  for (k=k2=0; k < n4; ++k,k2 += 2) {
    X[k]      = v[k2+n2]*B[k2  ] + v[k2+1+n2]*B[k2+1];
    X[n2-1-k] = v[k2+n2]*B[k2+1] - v[k2+1+n2]*B[k2  ];
  }

  // decode kernel to output
  // determined the following value experimentally
  // (by first figuring out what made inverse_mdct_slow work); then matching that here
  // (probably vorbis encoder premultiplies by n or n/2, to save it on the decoder?)
  s = 0.5; // theoretically would be n4

  // [[[ note! the s value of 0.5 is compensated for by the B[] in the current code,
  //     so it needs to use the "old" B values to behave correctly, or else
  //     set s to 1.0 ]]]
  for (i=0; i < n4  ; ++i) buffer[i] = s * X[i+n4];
  for (   ; i < n3_4; ++i) buffer[i] = -s * X[n3_4 - i - 1];
  for (   ; i < n   ; ++i) buffer[i] = -s * X[i - n3_4];
}

MDCT* MDCT::get(int numBits) {
  static std::unordered_map<int, MDCT> cache;
  if (!cache.count(numBits)) {
    cache.emplace(numBits, numBits);
  }
  return &cache.at(numBits);
}

MDCT::MDCT(int numBits) : numBits(numBits), mdctSize(1 << numBits) {
  // Precompute twiddle tables
  int n = mdctSize, n2 = n >> 1, n4 = n >> 2, n8 = n >> 4;
  A.reserve(n2);
  B.reserve(n2);
  C.reserve(n4);

  float alpha = M_PI / n;
  for (int k = 0, k2 = 0; k < n4; ++k, k2 += 2) {
    A.push_back(cos(4 * k * alpha));
    A.push_back(-sin(4 * k * alpha));
    float beta = (k2 + 1) * alpha;
    B.push_back(cos(beta * 0.5f) * 0.5f);
    B.push_back(sin(beta * 0.5f) * 0.5f);
    if (k < n8) {
      C.push_back(cos(beta * 2));
      C.push_back(-sin(beta * 2));
    }
  }

  // Precompute bit-reverse table
  int shift = 32 - numBits + 3;
  for (uint32_t i = 0; i < n4; ++i) {
    uint32_t br = ((i & 0xAAAAAAAA) >>  1) | ((i & 0x55555555) << 1);
    br = ((br & 0xCCCCCCCC) >>  2) | ((br & 0x33333333) << 2);
    br = ((br & 0xF0F0F0F0) >>  4) | ((br & 0x0F0F0F0F) << 4);
    br = ((br & 0xFF00FF00) >>  8) | ((br & 0x00FF00FF) << 8);
    bitReverse.push_back(((br >> 16) | (br << 16)) >> shift);
  }
}

void MDCT::calcInverse(float* coefs, std::vector<float>& output) const
{
  output.resize(mdctSize);
  std::memcpy(output.data(), coefs, mdctSize);
  inverse_mdct(output.data());
}
