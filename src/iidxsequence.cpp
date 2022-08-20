#include "iidxsequence.h"
#include "asfcodec.h"
#include "wmacodec.h"
#include "codec/sampledata.h"
#include "codec/riffcodec.h"
#include "riffwriter.h"
#include "utility.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

IIDXSequence::IIDXSequence(const std::string& path, const OpenFn& openFile) : openFile(openFile)
{
  int dotPos = path.rfind('.');
  if (dotPos == std::string::npos) {
    throw std::runtime_error("Invalid path specified");
  }
  basePath = path.substr(0, dotPos + 1);

  auto seqFile = openFile(basePath + "1");
  addTrack(new OneTrack(*seqFile.get()));
}

double IIDXSequence::duration() const
{
  return tracks.at(0)->length();
}

SynthContext* IIDXSequence::initContext()
{
  bool hasSamples = loadS3P() || load2DX();
  if (!hasSamples) {
    throw std::runtime_error("No sample data found");
  }
  int sampleRate = 44100;
  ctx.reset(new SynthContext(sampleRate));
  ctx->addChannel(getTrack(0));
  return ctx.get();
}

bool IIDXSequence::loadS3P()
{
  SampleData::purge();
  AsfCodec wmaCodec;
  try {
    auto file = openFile(basePath + "s3p");
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
        wmaCodec.decode(wmaData, samplesRead + 1);
      } catch (WmaException& w) {
        std::cerr << "Ignoring error in sample #" << samplesRead << ": " << w.what() << std::endl;
#ifndef NDEBUG
        std::ofstream f("sample" + std::to_string(samplesRead) + ".wma", std::ios::binary | std::ios::out | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(wmaData.data()), wmaData.size());
        f.close();
#endif
      }
      samplesRead++;
    }
    // Succeeded if all samples were read
    return samplesRead == numSamples;
  } catch (...) {
    // In case of any errors (including file not found) return failure
    return false;
  }
}

bool IIDXSequence::load2DX()
{
  SampleData::purge();
  RiffCodec riffCodec;
  try {
    std::cerr << "Reading " << basePath << "2dx..." << std::endl;
    auto file = openFile(basePath + "2dx");
    file->ignore(20);
    std::vector<char> buffer(12);
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
    int riffOffset;
    while (file && samplesRead < numSamples) {
      if (!file->seekg(offsets[samplesRead])) {
        return false;
      }
      if (!file->read(buffer.data(), 12) || parseIntBE<uint32_t>(buffer, 0) != '2DX9') {
        return false;
      }
      riffOffset = parseInt<uint32_t>(buffer, 4);
      std::vector<uint8_t> riffData(parseInt<uint32_t>(buffer, 8));
      file->ignore(riffOffset - 12);
      if (!file->read(reinterpret_cast<char*>(riffData.data()), riffData.size())) {
        return false;
      }
      riffCodec.decode(riffData, samplesRead + 1);
      samplesRead++;
    }
    // Succeeded if all samples were read
    return samplesRead == numSamples;
  } catch (std::exception& e) {
    // In case of any errors (including file not found) return failure
    std::cerr << e.what() << std::endl;
    return false;
  }
}
