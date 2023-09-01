#include "bankloaders.h"
#include "s2wcontext.h"
#include "codec/riffcodec.h"
#include "wma/asfcodec.h"
#include "wma/wmacodec.h"
#include <iostream>

bool loadS3P(S2WContext* ctx, std::istream* file, uint64_t space)
{
  AsfCodec wmaCodec(ctx);
  std::vector<char> buffer(12);
  if (!file->read(buffer.data(), 8)) {
    return false;
  }
  if (parseIntBE<uint32_t>(buffer, 0) != 'S3P0') {
    return false;
  }
  uint32_t numSamples = parseInt<uint32_t>(buffer, 4);
  std::vector<uint32_t> offsets;
  offsets.reserve(numSamples);
  for (int i = 0; file && i < numSamples; i++) {
    file->read(buffer.data(), 8);
    offsets.push_back(parseInt<uint32_t>(buffer, 0));
  }
  int samplesRead = 0;
  int wmaOffset;
  while (file && samplesRead < numSamples) {
    if (!file->seekg(offsets[samplesRead])) {
      return false;
    }
    if (!file->read(buffer.data(), 12) || parseIntBE<uint32_t>(buffer, 0) != 'S3V0') {
      return false;
    }
    wmaOffset = parseInt<uint32_t>(buffer, 4);
    std::vector<uint8_t> wmaData(parseInt<uint32_t>(buffer, 8));
    file->ignore(wmaOffset - 12);
    if (!file->read(reinterpret_cast<char*>(wmaData.data()), wmaData.size())) {
      return false;
    }
    try {
      wmaCodec.decode(wmaData, (samplesRead + 1) | space);
    } catch (WmaException& w) {
      std::cerr << "Ignoring error in sample #" << samplesRead << ": " << w.what() << std::endl;
    }
    samplesRead++;
  }
  // Succeeded if all samples were read
  return samplesRead == numSamples;
}

bool load2DX(S2WContext* ctx, std::istream* file, uint64_t space)
{
  RiffCodec riffCodec(ctx);
  file->ignore(20);
  std::vector<char> buffer(18);
  if (!file->read(buffer.data(), 4)) {
    return false;
  }
  uint32_t numSamples = parseInt<uint32_t>(buffer, 0);
  file->ignore(48);
  std::vector<int> offsets;
  offsets.reserve(numSamples);
  for (int i = 0; file && i < numSamples; i++) {
    file->read(buffer.data(), 4);
    offsets.push_back(parseInt<uint32_t>(buffer, 0));
  }
  int samplesRead = 0;
  int bgSamplesRead = 0;
  int riffOffset;
  while (file && (samplesRead + bgSamplesRead) < numSamples) {
    if (!file->seekg(offsets[samplesRead])) {
      return false;
    }
    if (!file->read(buffer.data(), 18) || parseIntBE<uint32_t>(buffer, 0) != '2DX9') {
      return false;
    }
    uint64_t sampleID = (samplesRead + 1) | space;
    riffOffset = parseInt<uint32_t>(buffer, 4);
    uint16_t sampleType = parseInt<uint16_t>(buffer, 12);
    // 0x3231 appears to be IIDX
    // 0x3230 appears to be pop'n
    // SDVX apparently also uses .2dx?
    // Supposedly there's a keysound ID in the header somewhere but none of the files I've seen use it
    if (sampleType == 0x3230) {
      uint16_t tracks = parseInt<uint16_t>(buffer, 14);
      //std::cerr << tracks << "\t";
      if (tracks == 0x0000) {
        sampleID = 0x10000 | space | (bgSamplesRead + 1);
      }
    }
    //std::cerr << sampleID << " @ offset " << offsets[samplesRead] << ": " << std::hex << sampleType << std::dec << std::endl;
    std::vector<uint8_t> riffData(parseInt<uint32_t>(buffer, 8));
    file->ignore(riffOffset - 18);
    if (!file->read(reinterpret_cast<char*>(riffData.data()), riffData.size())) {
      return false;
    }
    riffCodec.decode(riffData, sampleID);
    if (sampleID & 0x10000) {
      bgSamplesRead++;
    } else {
      samplesRead++;
    }
  }
  // Succeeded if all samples were read
  return samplesRead == numSamples;
}
