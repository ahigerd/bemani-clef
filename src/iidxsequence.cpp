#include "iidxsequence.h"
#include "clefcontext.h"
#include "codec/sampledata.h"
#include "codec/riffcodec.h"
#include "utility.h"
#include "bankloaders.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>

IIDXSequence::IIDXSequence(ClefContext* ctx, const std::string& path)
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
  try {
    std::cerr << "Reading " << basePath << "s3p..." << std::endl;
    auto file = context()->openFile(basePath + "s3p");
    return ::loadS3P(context(), file.get());
  } catch (...) {
    // In case of any errors (including file not found) return failure
    return false;
  }
}

bool IIDXSequence::load2DX()
{
  context()->purgeSamples();
  try {
    std::cerr << "Reading " << basePath << "2dx..." << std::endl;
    auto file = context()->openFile(basePath + "2dx");
    return ::load2DX(context(), file.get());
  } catch (std::exception& e) {
    // In case of any errors (including file not found) return failure
    std::cerr << e.what() << std::endl;
    return false;
  }
}
