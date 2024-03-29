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

#include "wmacodec.h"
#include "tagmap.h"
#include "utility.h"
#include "wmadata.h"
#include "bitstream.h"
#include "sintable.h"
#include "mdct.h"
#include "wma_vlc.h"
#include "codec/riffcodec.h"
#include <unordered_map>
#include <utility>
#include <memory>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <array>
#include <cmath>
#include <complex>

// from wma.c:
static inline int ff_wma_total_gain_to_bits(int total_gain)
{
    if (total_gain < 15)
        return 13;
    else if (total_gain < 32)
        return 12;
    else if (total_gain < 40)
        return 11;
    else if (total_gain < 45)
        return 10;
    else
        return  9;
}

WmaCodec::WmaCodec(ClefContext* ctx, const WaveFormatEx& fmt, uint32_t maxPacketSize)
: ICodec(ctx), fmt(fmt), maxPacketSize(maxPacketSize)
{
  std::memset(exponents, 0, sizeof(exponents));
  std::memset(coefs1, 0, sizeof(coefs1));
  std::memset(coefs, 0, sizeof(coefs));
  maxExponent[0] = maxExponent[1] = 0;
  expBits[0] = expBits[1] = 0;

  frameBits = fmt.sampleRate > 22050 ? 11 : 10;
  frameLen = 1 << frameBits;

  lastBlockSize = frameLen;
  blockBits = frameBits;

  // WMAv2 only, exponents as VLC only, use bit reservoir, no noise coding, default max freq, VLC tables 4/5
  // TODO: implement other variations if they're ever needed
  uint16_t flags = parseInt<uint16_t>(fmt.exData, fmt.format == 0x0162 ? 6 : 4);
  bool expVLC = flags & 1;
  bitReservoir = flags & 2;
  bool useVBR = flags & 4;
  if (!bitReservoir || !expVLC || !(fmt.sampleRate == 22050 || fmt.sampleRate == 32000 || fmt.sampleRate == 44100)) {
    throw WmaException("Unsupported WMAv2");
  }

  if (useVBR) {
    int vbl = ((flags >> 3) & 3);
    bool isHighRate = (fmt.byteRate / fmt.channels) >= 4000;
    blockSizeBits = (vbl < 2 ? 1 : 2) + (isHighRate ? 2 : 0);
  } else {
    blockSizeBits = 0;
  }

  double bps = (fmt.byteRate * 8.0) / (fmt.channels * fmt.sampleRate);
  byteOffsetBits = std::log2(int(bps * frameLen / 8.0 + 0.5)) + 2;

  numCoefsBase = frameLen * .91 + .99;
}

SampleData* WmaCodec::decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID)
{
  sampleData = sampleID ? new SampleData(context(), sampleID) : new SampleData(context());
  sampleData->sampleRate = fmt.sampleRate;
  for (int i = 0; i < fmt.channels; i++) {
    sampleData->channels.emplace_back(0);
  }

  coefVlc[0] = VLC::get(4);
  coefVlc[1] = VLC::get(5);
  expVlc = VLC::get(-1);
  mdct = MDCT::get(frameBits + 1);

#ifdef _MSC_VER
  // XXX: Working around what appears to be a compiler error
  BitStream bitstream(&*start, &*end, maxPacketSize);
#else
  BitStream bitstream(start, end, maxPacketSize);
#endif
  expBits[0] = expBits[1] = frameBits;
  {
    const auto& rawTables = fmt.sampleRate == 22050 ? exponent_band_22050 :
      fmt.sampleRate == 32000 ? exponent_band_32000 :
      exponent_band_44100;
    bandTables.push_back(std::vector<uint16_t>());
    bandTables.push_back(std::vector<uint16_t>());
    int last = 0;
    for (int i = 0; i < 25; i++) {
      int p1 = std::min(frameLen, ((ff_wma_critical_freqs[i] * 2 * frameLen) + (fmt.sampleRate * 2)) / (4 * fmt.sampleRate) * 4);
      bandTables[0].push_back(p1 - last);
      last = p1;
    }
    last = 0;
    for (int i = 0; i < 25; i++) {
      int p1 = std::min(frameLen / 2, ((ff_wma_critical_freqs[i] * frameLen) + (fmt.sampleRate * 2)) / (4 * fmt.sampleRate) * 4);
      bandTables[1].push_back(p1 - last);
      last = p1;
    }
    for (int i = 2; i >= 0; --i) {
      bandTables.emplace_back(rawTables[i] + 1, rawTables[i] + 1 + rawTables[i][0]);
    }
  }

  samplesDone = 0;
  while (bitstream.remaining()) {
    parseSuperframe(bitstream);
    bitstream.nextPacket();
  }

  return sampleData;
}

void WmaCodec::parseSuperframe(BitStream& bitstream)
{
  bitstream.resetBitsConsumed();
  int sfID = bitstream.read(4);
  int numFrames = bitReservoir ? bitstream.read(4) : 1;
  if (bitReservoir && bitstream.bitsReserved() <= (!numFrames ? 8 : 0)) {
    numFrames--;
  }
  if (numFrames < 0) {
    throw WmaException("bad frame count in superframe");
  }

  for (int i = 0; i < fmt.channels; i++) {
    sampleData->channels[i].resize(sampleData->channels[i].size() + (numFrames + 1) * frameLen);
  }
  int bitOffset = bitstream.read(byteOffsetBits + 3);
  if (bitOffset > bitstream.remaining()) {
    throw WmaException("invalid bit offset");
  }

  int pos = bitOffset + byteOffsetBits + 11;
  if (pos > fmt.blockAlign << 3) {
    throw WmaException("invalid bit offset");
  }

  int startFrame = 0;
  hasReservedFrame = bitReservoir && bitstream.bitsReserved();
  if (hasReservedFrame) {
    numFrames--;
    startFrame = -1;
  }
  for (int i = startFrame; i < numFrames; i++) {
    if (blockSizeBits && i == 0) {
      lastBlockSize = 1 << (frameBits - bitstream.read(blockSizeBits));
      int bitsRead = bitstream.read(blockSizeBits);
      blockBits = frameBits - bitsRead;
    }
    parseFrame(bitstream, i);
  }
  if (bitReservoir) {
    bitstream.reserve(fmt.blockAlign * 8 - bitstream.bitsConsumed());
  }
}

void WmaCodec::parseFrame(BitStream& bitstream, int frameNum)
{
  if (frameNum < 0) {
    bitstream.useReserved();
  }
  int bitsRemaining = frameLen;
  int blockNum = 0;
  while (bitsRemaining > 0) {
    parseBlock(bitstream, frameNum, blockNum);
    ++blockNum;
    bitsRemaining -= lastBlockSize;
  }
  if (frameNum < 0) {
    bitstream.flushReserved();
  }
}

void WmaCodec::parseBlock(BitStream& bitstream, int frameNum, int blockNum)
{
  int blockSize = 1 << blockBits;
  int nextBlockBits = blockSizeBits ? frameBits - bitstream.read(blockSizeBits) : frameBits;
  int nextBlockSize = 1 << nextBlockBits;
  bool isMsStereo = fmt.channels == 2 ? bitstream.read(1) : false;
  bool channelCoded[2] = { false, false };
  for (int ch = 0; ch < fmt.channels; ch++) {
    channelCoded[ch] = bitstream.read(1);
  }
  if (channelCoded[0] || channelCoded[1]) {
    float totalGain = 1.0;
    int a;
    do {
      a = bitstream.read(7);
      totalGain += a;
    } while (a == 127);

    bool hasExponents = (blockSize == frameLen) ? true : bitstream.read(1);
    if (hasExponents) {
      for (int ch = 0; ch < fmt.channels; ch++) {
        if (channelCoded[ch]) {
          expBits[ch] = frameBits - blockBits;
          const auto& bandTable = bandTables[expBits[ch]];
          int index = 36;
          maxExponent[ch] = 0;
          for (int j = 0, pos = 0; pos < blockSize; j++) {
            int code = expVlc->extractFrom(bitstream) - 60;
            index += code;
            float value = std::pow(10.0, index / 16.0);
            if (value > maxExponent[ch]) {
              maxExponent[ch] = value;
            }
            for (int k = 0; pos < blockSize && k < bandTable[j]; k++, pos++) {
              exponents[ch][pos] = value;
            }
          }
        }
      }
    }

    int coefNumBits = ff_wma_total_gain_to_bits(totalGain);
    numCoefs = numCoefsBase >> (frameBits - blockBits);

    for (int ch = 0; ch < fmt.channels; ch++) {
      if (channelCoded[ch]) {
        float* ptr = coefs1[ch];
        int tindex = (ch == 1 && isMsStereo) ? 1 : 0;
        VLC* vlc = coefVlc[tindex];
        std::memset(ptr, 0, frameLen * sizeof(float));
        int sign, offset;
        uint32_t coefMask = frameLen - 1;
        for (offset = 0; offset < numCoefs; offset++) {
          int code = vlc->extractFrom(bitstream);
          if (code > 1) {
            offset += vlc->runTable[code];
            sign = bitstream.read(1) - 1;
            float level = vlc->levelTable[code] * (sign & 0x80000000 ? -1 : 1);
            ptr[offset & coefMask] = level;
          } else if (code == 1) {
            // end of block
            break;
          } else {
            // escape sequence
            int level = bitstream.read(coefNumBits);
            offset += bitstream.read(frameBits);
            int sign = bitstream.read(1) - 1;
            ptr[offset & coefMask] = (level ^ sign) - sign;
          }
        }
        if (offset > numCoefs) {
          throw WmaException("RLE overflow");
        }
      }
    }

    float mdctNorm = 2.0 / blockSize;
    for (int ch = 0; ch < fmt.channels; ch++) {
      if (!channelCoded[ch]) {
        continue;
      }
      float mult = mdctNorm * std::pow(10, totalGain * 0.05) / maxExponent[ch];
      std::memset(coefs[ch], 0, sizeof(coefs[ch]));
      for (int j = 0; j < numCoefs; j++) {
        coefs[ch][j] = coefs1[ch][j] * exponents[ch][j << (frameBits - blockBits) >> expBits[ch]] * mult;
      }
    }
    if (isMsStereo && channelCoded[1]) {
      if (!channelCoded[0]) {
        std::memset(coefs[0], 0, sizeof(coefs[0]));
        channelCoded[0] = true;
      }
      // butterfly
      for (int j = 0; j < frameLen; j++) {
        float t = coefs[0][j] - coefs[1][j];
        coefs[0][j] += coefs[1][j];
        coefs[1][j] = t;
      }
    }
  }

  MDCT* mdct = MDCT::get(blockBits + 1);
  int fadeIn = std::min(blockSize, lastBlockSize);
  SinTable* sin = SinTable::get(fadeIn);
  int numSamples = blockSize * 2;
  for (int ch = 0; ch < fmt.channels; ch++) {
    if (channelCoded[ch]) {
      auto& output = sampleData->channels[ch];
      std::vector<float> samples(numSamples, 0);
      mdct->calcInverse(coefs[ch], samples);
      int i = (blockSize > lastBlockSize) ? (blockSize - lastBlockSize) >> 1 : 0;
      int j = i + samplesDone + ((frameLen - blockSize) >> 1);
      int fadeInSample = 0;
      for (; fadeInSample < fadeIn; i++, j++, fadeInSample++) {
        int32_t outSample = output[j] * sin->floatOut(fadeInSample);
        int32_t inSample = samples[i] * sin->intIn(fadeInSample);
        output[j] = clamp<int16_t>(outSample + inSample, -0x7FFF, 0x7FFF);
      }
      for (; i < numSamples; i++, j++) {
        output[j] = samples[i] * 0x7FFF;
      }
    }
  }

  samplesDone += blockSize;
  lastBlockSize = blockSize;
  blockBits = nextBlockBits;
}

WmaException::WmaException(const std::string& message)
: std::runtime_error(message)
{
  // initializers only
}
