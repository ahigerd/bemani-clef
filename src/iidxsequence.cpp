#include "iidxsequence.h"
#include "wma/asfcodec.h"
#include "wma/wmacodec.h"
#include "s2wcontext.h"
#include "codec/sampledata.h"
#include "codec/riffcodec.h"
#include "riffwriter.h"
#include "utility.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

IIDXSequence::IIDXSequence(S2WContext* ctx, const std::string& path)
: BaseSequence(ctx)
{
  int dotPos = path.rfind('.');
  if (dotPos == std::string::npos) {
    throw std::runtime_error("Invalid path specified");
  }
  basePath = path.substr(0, dotPos + 1);

  auto seqFile = ctx->openFile(basePath + "1");
  addTrack(new OneTrack(*seqFile.get()));
}

double IIDXSequence::duration() const
{
  return tracks.at(0)->length();
}

SynthContext* IIDXSequence::initContext()
{
  int sampleRate = 44100;
  try {
    synth.reset(new SynthContext(context(), sampleRate));
    bool hasSamples = loadS3P() || load2DX();
    if (!hasSamples) {
      throw std::runtime_error("No sample data found");
    }
    synth->addChannel(getTrack(0));
    return synth.get();
  } catch (...) {
    synth.reset(nullptr);
    throw;
  }
}

bool IIDXSequence::loadS3P()
{
  context()->purgeSamples();
  AsfCodec wmaCodec(context());
  try {
    auto file = context()->openFile(basePath + "s3p");
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
  context()->purgeSamples();
  RiffCodec riffCodec(context());
  try {
    std::cerr << "Reading " << basePath << "2dx..." << std::endl;
    auto file = context()->openFile(basePath + "2dx");
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
