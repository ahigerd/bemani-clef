#include "ifssequence.h"
#include "ifs.h"
#include "va3.h"
#include "sq3track.h"
#include "sq2track.h"
#include "phasetrack.h"
#include "codec/adpcmcodec.h"
#include "codec/sampledata.h"
#include "../bmpcodec.h"
#include "utility.h"
#include "synth/synthcontext.h"
#include <numeric>
#include <iostream>
#include <sstream>

uint64_t IFSSequence::stringToSpaces(const std::string& channels)
{
  uint64_t spaces = 0;
  for (char ch : channels) {
    switch (ch) {
      case 'd':
      case 'D':
        spaces |= SampleSpaces::Drums;
        break;
      case 'g':
      case 'G':
        spaces |= SampleSpaces::Guitar;
        break;
      case 'b':
      case 'B':
        spaces |= SampleSpaces::Bass;
        break;
      case 'k':
      case 'K':
        spaces |= SampleSpaces::Keyboard;
        break;
      case 's':
      case 'S':
        spaces |= SampleSpaces::Backing;
        break;
    }
  }
  return spaces;
}

IFSSequence::IFSSequence(S2WContext* ctx)
: BaseSequence<ITrack>(ctx), sampleRate(48000), mute(0)
{
  // initializers only
}

void IFSSequence::addIFS(IFS* ifs)
{
  files.emplace_back(ifs);
}

void IFSSequence::load()
{
  std::unordered_map<uint64_t, std::string> streams;
  std::vector<std::string> seqFiles;
  bool useSQ3 = false;
  uint32_t sequences = 0;

  for (const auto& ifs : files) {
    // Pass 1: samples
    for (auto iter : ifs->files) {
      const auto& filename = iter.first;
      int extPos = filename.rfind(".");
      if (extPos == std::string::npos) {
        // File without extension -- ignore
        continue;
      }
      std::string extension = filename.substr(extPos + 1);
      if (extension == "va3") {
        // Preload samples
        int sampleSpace = stringToSpaces(filename.substr(extPos - 1, 1));
        VA3 va3(ifs.get(), filename);
        for (auto iter2 : va3.files) {
          auto span = va3.get(iter2.first);
          AdpcmCodec codec(context(), AdpcmCodec::OKI4s, iter2.second.channels > 1 ? -1 : 0);
          SampleData* sample = codec.decodeRange(span.first, span.second, sampleSpace | iter2.second.sampleID);
          sampleData[sampleSpace | iter2.second.sampleID] = iter2.second;
          std::istringstream ss(iter2.first, std::ios::in);
          int fileNumber;
          ss >> std::hex >> fileNumber;
          if (!ss.fail()) {
            uint64_t fnID = SampleSpaces::ByFilename | sampleSpace | fileNumber;
            if (!sampleData.count(fnID)) {
              sampleData[fnID] = iter2.second;
            }
          }
          sample->sampleRate = iter2.second.sampleRate;
        }
        for (auto iter2 : va3.defaultDrums) {
          sampleData[SampleSpaces::ByNote | sampleSpace | iter2.first] = sampleData[sampleSpace | iter2.second];
        }
      } else if (extension == "bin" && filename.substr(0, 3) == "bgm") {
        size_t pos = filename.rfind('.');
        int streamType = stringToSpaces(filename.substr(pos - 4, 4)) | SampleSpaces::Backing;
        streams[streamType] = filename;
      } else if (extension.find("sq") == 0) {
        useSQ3 = useSQ3 || filename.back() == '3';
        seqFiles.push_back(filename);
      }
    }
  }

  for (const auto& ifs : files) {
    // Pass 2: sequences
    for (const std::string filename : seqFiles) {
      const auto& dataIter = ifs->files.find(filename);
      if (dataIter == ifs->files.end()) {
        // Named file is not in this IFS
        continue;
      }
      const auto& data = dataIter->second;
      int sampleSpace = stringToSpaces(filename.substr(0, 1));
      if (filename[filename.size() - 1] == '3') {
        if (useSQ3) {
          if (!(sampleSpace & mute)) {
            addTrack(new Sq3Track(this, &data[0], data.size(), sampleSpace));
          }
          sequences |= sampleSpace;
        }
      } else if (filename[filename.size() - 1] == '2') {
        if (!useSQ3) {
          if (!(sampleSpace & mute)) {
            addTrack(new Sq2Track(this, &data[0], data.size(), sampleSpace));
          }
          sequences |= sampleSpace;
        }
      } else {
        std::cerr << "Warning: unknown sequence type: " << filename << std::endl;
      }
    }
  }

  if (!sequences) {
    usePhasedStreams(streams);
    return;
  }

  if (mute & SampleSpaces::Backing) {
    // user doesn't want the backing track
    return;
  }

  const IFS* streamSource = nullptr;
  std::string streamFilename;
  int streamScore = 0;
  // Pass 3: streams
  for (const auto& iter : streams) {
    uint64_t streamType = iter.first;
    int score = 0;
    if (((streamType | sequences) & 0xF0000) == 0xF0000) {
      // all parts are covered, pick the combination that uses the most sequenced tracks
      score = countBits(0xF0000 & ~streamType) + 100;
    } else {
      // not all parts are covered, pick the combination that covers the most parts, at a penalty
      score = countBits(0xF0000 & (streamType | sequences));
    }
    if (score > streamScore) {
      streamFilename = iter.second;
      streamScore = score;
    }
  }

  if (streamScore) {
    BmpCodec codec(context());
    SampleData* sample = nullptr;
    for (const auto& ifs : files) {
      auto iter = ifs->files.find(streamFilename);
      if (iter == ifs->files.end()) {
        continue;
      }
      sample = codec.decode(iter->second);
      break;
    }
    if (!sample) {
      std::cerr << "Unable to find stream: " << streamFilename << std::endl;
      return;
    }
    BasicTrack* track = new BasicTrack;
    SampleEvent* event = new SampleEvent;
    event->sampleID = sample->sampleID;
    event->timestamp = 0;
    event->duration = sample->duration();
    // TODO: is the volume stored somewhere?
    event->volume = 2.0;
    track->addEvent(event);
    addTrack(track);
  }
}

void IFSSequence::setMutes(const std::string& channels)
{
  uint64_t spaces = stringToSpaces(channels);
  mute = 0x1F0000 & spaces;
}

void IFSSequence::setSolo(const std::string& channels)
{
  uint64_t spaces = stringToSpaces(channels);
  mute = 0x1F0000 & ~spaces;
}

struct ScoreResult {
  uint32_t score;
  std::vector<uint64_t> streams;
};

static ScoreResult scoreCombination(uint64_t mute, uint32_t base, std::vector<uint64_t> add, ScoreResult before = { 0, {} })
{
  ScoreResult result{ 0, {} };
  bool isLast = add.size() == 1;
  int neededChannels = 4 - countBits(mute);
  for (uint64_t streamID : add) {
    if (streamID & mute) {
      continue;
    }
    bool invert = streamID & SampleSpaces::Invert;
    uint32_t v = (streamID >> 16) & 0xF;
    uint32_t score = 0;
    uint32_t chanSum = 0;
    int goodChannels = 0;
    bool reject = false;
    for (int i = 0; i < 4; i++) {
      uint8_t chanScore = (v >> i) & 0x01;
      uint8_t baseScore = (base >> (i * 8)) & 0xFF;
      if (chanScore && invert == !(baseScore & 1)) {
        // Don't subtract a channel that isn't already there
        reject = true;
        break;
      }
      chanScore += baseScore;
      chanSum |= chanScore << (i * 8);
      if (isLast && chanScore & 1 == 0) {
        // If any channels would be left out, this combination is bad
        break;
      } else if (chanScore == 1) {
        // If a channel gets represented once, that's best
        score += 10;
        goodChannels++;
      } else if (chanScore & 1 == 1) {
        // If a channel gets represented an odd number of times, that's acceptable,
        // but 3 is better than 5 or higher
        score += (chanScore == 3 ? 7 : 6);
        goodChannels++;
      }
    }
    if (reject) {
      continue;
    }
    if (goodChannels < neededChannels) {
      // Some channels are missing, recurse
      std::vector<uint64_t> rest;
      for (uint64_t v : add) {
        if ((v & ~SampleSpaces::Invert) != (streamID & ~SampleSpaces::Invert)) {
          rest.push_back(v);
        }
      }
      ScoreResult recScore = scoreCombination(mute, chanSum, rest, result);
      if (recScore.score > result.score) {
        result = recScore;
        result.streams.push_back(streamID);
      }
    } else if (score > result.score) {
      result.score = score;
      result.streams = before.streams;
      result.streams.push_back(streamID);
    }
  }
  return result;
}

void IFSSequence::usePhasedStreams(const std::unordered_map<uint64_t, std::string>& streams)
{
  std::vector<uint64_t> allStreams;
  for (const auto& iter : streams) {
    allStreams.push_back(iter.first);
    allStreams.push_back(SampleSpaces::Invert | iter.first);
  }
  ScoreResult result = scoreCombination(mute & 0xF0000, 0, allStreams);
  if (result.streams.empty()) {
    return;
  }
  for (uint64_t sampleID : result.streams) {
    sampleID &= ~SampleSpaces::Invert;
    BmpCodec codec(context());
    for (const auto& ifs : files) {
      auto iter = ifs->files.find(streams.at(sampleID));
      if (iter != ifs->files.end()) {
        codec.decode(iter->second, sampleID);
        break;
      }
    }
  }
  addTrack(new PhaseTrack(context(), result.streams));
}

SynthContext* IFSSequence::initContext()
{
  ctx.reset(new SynthContext(context(), sampleRate));
  for (int i = 0; i < numTracks(); i++) {
    ctx->addChannel(getTrack(i));
  }
  return ctx.get();
}

double IFSSequence::duration() const
{
  double maxLength = 0;

  for (const auto& ifs : files) {
    for (auto iter : ifs->files) {
      const auto& filename = iter.first;
      int extPos = filename.rfind(".");
      if (extPos == std::string::npos) {
        // File without extension -- ignore
        continue;
      }
      std::string extension = filename.substr(extPos + 1);
      if (extension == "va3") {
        // Scan samples
        VA3 va3(ifs.get(), filename);
        for (auto iter2 : va3.files) {
          auto span = va3.get(iter2.first);
          double len = (span.second - span.first) * (iter2.second.channels > 1 ? 1.0 : 2.0) / iter2.second.sampleRate;
          if (len > maxLength) {
            maxLength = len;
          }
        }
      } else if (extension == "bin" && filename.substr(0, 3) == "bgm") {
        auto& data = iter.second;
        int channels = data[16];
        double sampleRate = parseIntBE<int32_t>(data.begin(), 20);
        double len = (data.size() - 32) * (channels > 1 ? 1.0 : 2.0) / sampleRate;
        if (len > maxLength) {
          maxLength = len;
        }
      } else if (extension.find("sq") == 0) {
        bool isSQ3 = filename.back() == '3';
        double len = isSQ3 ? Sq3Track::length(iter.second.data(), iter.second.size()) : Sq2Track::length(iter.second.data(), iter.second.size());
        if (len > maxLength) {
          maxLength = len;
        }
      }
    }
  }
  return maxLength;
}
