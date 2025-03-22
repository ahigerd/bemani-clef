#include "bankloaders.h"
#include "clefcontext.h"
#include "codec/riffcodec.h"
#include "wma/asfcodec.h"
#include "wma/wmacodec.h"
#include <iostream>

int loadS3P(ClefContext* ctx, std::istream* file, uint64_t space)
{
  AsfCodec wmaCodec(ctx);
  std::vector<char> buffer(12);
  if (!file->read(buffer.data(), 8)) {
    return 0;
  }
  if (parseIntBE<uint32_t>(buffer, 0) != 'S3P0') {
    return 0;
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
    //std::cerr << samplesRead << " loading " << offsets[samplesRead] << std::endl;
    if (!file->seekg(offsets[samplesRead])) {
      return 0;
    }
    if (!file->read(buffer.data(), 12) || parseIntBE<uint32_t>(buffer, 0) != 'S3V0') {
      return 0;
    }
    wmaOffset = parseInt<uint32_t>(buffer, 4);
    std::vector<uint8_t> wmaData(parseInt<uint32_t>(buffer, 8));
    file->ignore(wmaOffset - 12);
    if (!file->read(reinterpret_cast<char*>(wmaData.data()), wmaData.size())) {
      return 0;
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

static std::vector<int> get2DXSampleOffsets(std::istream* file)
{
  std::vector<int> offsets;
  std::vector<char> buffer(4);
  if (!file->read(buffer.data(), 4)) {
    return offsets;
  }
  uint32_t offsetBase = 0;
  if (buffer[0] == '%') {
    file->ignore(24);
    offsetBase = 8;
  } else {
    file->ignore(16);
  }
  if (!file->read(buffer.data(), 4)) {
    return offsets;
  }
  uint32_t numSamples = parseInt<uint32_t>(buffer, 0);
  file->ignore(48);
  offsets.reserve(numSamples);
  for (int i = 0; file && i < numSamples; i++) {
    file->read(buffer.data(), 4);
    offsets.push_back(parseInt<uint32_t>(buffer, 0) + offsetBase);
  }
  return offsets;
}

int load2DX(ClefContext* ctx, std::istream* file, uint64_t space, uint64_t onlySample)
{
  std::vector<int> offsets = get2DXSampleOffsets(file);
  int numSamples = offsets.size();
  if (!numSamples) {
    return 0;
  }
  std::vector<char> buffer(18);
  int samplesRead = 0;
  int bgSamplesRead = 0;
  int riffOffset;
  RiffCodec riffCodec(ctx);
  while (file && samplesRead < numSamples) {
    if (!offsets[samplesRead]) {
      // Sample table entry is null
      samplesRead++;
      continue;
    }
    if (onlySample && samplesRead != onlySample - 1) {
      samplesRead++;
      continue;
    }
    if (!file->seekg(offsets[samplesRead]) || !file->read(buffer.data(), 18)) {
      return 0;
    }
    uint32_t magic = parseIntBE<uint32_t>(buffer, 0);
    if (magic != '2DX9' && magic != 'SD9\0') {
      return 0;
    }
    uint64_t sampleID = (samplesRead + 1) | space;
    riffOffset = parseInt<uint32_t>(buffer, 4);
    uint16_t sampleType = parseInt<uint16_t>(buffer, 12);
    //std::cerr << sampleID << " @ offset " << offsets[samplesRead] << ": " << std::hex << sampleType << std::dec << std::endl;
    std::vector<uint8_t> riffData(parseInt<uint32_t>(buffer, 8));
    file->ignore(riffOffset - 18);
    if (!file->read(reinterpret_cast<char*>(riffData.data()), riffData.size())) {
      return 0;
    }
    riffCodec.decode(riffData, sampleID);
    // 0x3231 appears to be IIDX
    // 0x3230 appears to be pop'n, but maybe IIDX system bgm
    // SDVX apparently also uses .2dx?
    // Supposedly there's a keysound ID in the header somewhere but none of the files I've seen use it
    if (sampleType == 0x3230) {
      uint16_t tracks = parseInt<uint16_t>(buffer, 14);
      //std::cerr << tracks << "\n";
      if (tracks == 0x0000) {
        // Keep track of the last background sample
        sampleID = 0x10001 | space;
        riffCodec.decode(riffData, sampleID);
      }
    }
    samplesRead++;
  }
  // Succeeded if all samples were read
  if (samplesRead == numSamples) {
    return samplesRead;
  }
  return 0;
}

std::vector<uint64_t> get2DXSampleIDs(ClefContext* ctx, std::istream* file, uint64_t space)
{
  std::vector<uint64_t> ids;
  std::vector<int> offsets = get2DXSampleOffsets(file);
  int numOffsets = offsets.size();
  if (!numOffsets) {
    return ids;
  }
  for (int i = 0; i < numOffsets; i++) {
    if (offsets[i]) {
      uint64_t sampleID = (i + 1) | space;
      ids.push_back(sampleID);
    }
  }
  return ids;
}

double get2DXSampleLength(std::istream* file, uint64_t sampleID)
{
  sampleID -= 1;
  std::vector<int> offsets = get2DXSampleOffsets(file);
  int numSamples = offsets.size();
  if (!numSamples || numSamples < sampleID || !offsets[sampleID]) {
    return 0;
  }
  std::vector<char> buffer(18);
  RiffCodec riffCodec(nullptr);
  if (!file->seekg(offsets[sampleID]) || !file->read(buffer.data(), 18)) {
    return 0;
  }
  uint32_t magic = parseIntBE<uint32_t>(buffer, 0);
  if (magic != '2DX9' && magic != 'SD9\0') {
    return 0;
  }
  int riffOffset = parseInt<uint32_t>(buffer, 4);
  uint16_t sampleType = parseInt<uint16_t>(buffer, 12);
  std::vector<uint8_t> riffData(parseInt<uint32_t>(buffer, 8));
  file->ignore(riffOffset - 18);
  if (!file->read(reinterpret_cast<char*>(riffData.data()), riffData.size())) {
    return 0;
  }
  std::unique_ptr<SampleData> sample(riffCodec.decode(riffData, SampleData::Uncached));
  if (!sample) {
    return 0;
  }
  return sample->duration();
}
