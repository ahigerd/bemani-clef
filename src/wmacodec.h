/*
 * WMA decoder copyright (c) 2020 Adam Higerd
 * FFmpeg copyright (c) 2000-2004 The FFmpeg Project, Fabrice Bellard,
 *     Michael Niedermayer <michaelni@gmx.at>
 *
 * bemani2wav is free software. Most of its source code is licensed
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
#ifndef B2W_WMACODEC_H
#define B2W_WMACODEC_H

#include "codec/riffcodec.h"
#include "bitstream.h"
#include <stdexcept>
class VLC;
class MDCT;

class WmaException : public std::runtime_error {
public:
  WmaException(const std::string& message);
};

class WmaCodec : public ICodec {
public:
  WmaCodec(const WaveFormatEx& fmt, uint32_t maxPacketSize);

  virtual SampleData* decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID = 0);

private:
  void parseSuperframe(BitStream& bitstream);
  void parseFrame(BitStream& bitstream, int frameNum);
  void parseBlock(BitStream& bitstream, int frameNum, int blockNum);

  WaveFormatEx fmt;
  SampleData* sampleData;
  VLC* coefVlc[2];
  VLC* expVlc;
  MDCT* mdct;
  uint32_t maxPacketSize;
  uint32_t frameBits, frameLen;
  uint32_t numCoefsBase, numCoefs;
  uint8_t blockSizeBits, byteOffsetBits;
  bool bitReservoir;
  bool hasReservedFrame;
  bool useReservedFrame;

  float coefs1[2][2048], coefs[2][2048];
  float exponents[2][2048], maxExponent[2];
  uint32_t expBits[2];
  int lastBlockSize, blockBits;
  int samplesDone;
  std::vector<std::vector<uint16_t>> bandTables;
};

#endif
