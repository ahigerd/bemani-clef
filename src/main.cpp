#include "ifs/ifs.h"
#include "ifs/ifssequence.h"
#include "bankloaders.h"
#include "identify.h"
#include "iidxsequence.h"
#include "riffwriter.h"
#include "clefcontext.h"
#include "synth/synthcontext.h"
#include "synth/channel.h"
#include "commandargs.h"
#include "tagmap.h"
#include "wma/asfcodec.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>

int writeSample(ClefContext* ctx, SampleData* sample, const std::string& filename)
{
  std::cerr << "Writing " << (int(sample->duration() * 10) * .1) << " seconds to \"" << filename << "\"..." << std::endl;
  int channels = sample->channels.size();
  RiffWriter riff(sample->sampleRate, channels > 1, sample->numSamples() * channels * 2);
#ifndef _WIN32
  riff.open(filename == "-" ? "/dev/stdout" : filename);
#else
  riff.open(filename);
#endif
  if (sample->channels.size() > 1) {
    riff.write(sample->channels[0], sample->channels[1]);
  } else {
    riff.write(sample->channels[0]);
  }
  riff.close();
  return 0;
}

int decodeWma(ClefContext* ctx, const std::string& infile, const std::string& filename)
{
  AsfCodec wma(ctx);
  return writeSample(ctx, wma.decodeFile(infile), filename);
}

void saveOutput(SynthContext* ctx, std::string filename)
{
#ifndef _WIN32
  if (filename == "-") {
    filename = "/dev/stdout";
  }
#endif
  std::cerr << "Writing " << (int(ctx->maximumTime() * 10) * .1) << " seconds to \"" << filename << "\"..." << std::endl;
  RiffWriter riff(ctx->sampleRate, true);
  riff.open(filename);
  ctx->save(&riff);
  riff.close();
}

int processIFS(CommandArgs& args, ClefContext& clef, const char* programName)
{
  IFSSequence seq(&clef, args.hasKey("preview"));
  if (args.hasKey("mute")) {
    seq.setMutes(args.getString("mute"));
  }
  if (args.hasKey("solo")) {
    seq.setSolo(args.getString("solo"));
  }
  std::vector<std::string> positional = args.positional();
  for (const std::string& fn : args.positional()) {
    std::string paired = IFS::pairedFile(fn);
    if (!paired.empty() && std::find(positional.begin(), positional.end(), paired) == positional.end()) {
      positional.push_back(paired);
    }
  }
  for (const std::string& fn : positional) {
    std::ifstream file(fn, std::ios::in | std::ios::binary);
    IFS* ifs = new IFS(file);
    if (args.hasKey("verbose")) {
      ifs->manifest.dump();
    }
    seq.addIFS(ifs);
  }
  seq.load();
  if (!seq.numTracks()) {
    std::cerr << programName << ": no playable tracks found, or all tracks muted" << std::endl;
    return 1;
  }

  for (const std::string& fn : positional) {
    std::string m3uPath = TagsM3U::relativeTo(fn);
    std::ifstream tags(m3uPath);
    if (!tags) {
      continue;
    }
    TagsM3U m3u(tags);
    int trackIndex = m3u.findTrack(fn);
    if (trackIndex >= 0) {
      std::cerr << "Tags found in " << m3uPath << ":" << std::endl;
      for (const auto& iter : m3u.allTags(trackIndex)) {
        std::cerr << iter.first << " = " << iter.second << std::endl;
      }
      break;
    }
  }

  SynthContext* ctx(seq.initContext());
  std::string filename = args.getString("output", args.positional()[0] + ".wav");
  saveOutput(ctx, filename);
  return 0;
}

int process2dxStream(CommandArgs& args, ClefContext& clef, const char* programName)
{
  std::string infile = args.positional().at(0);
  bool verbose = args.hasKey("verbose");
  int subsong = 0;
  if (args.hasKey("subsong")) {
    subsong = args.getInt("subsong");
  } else {
    int qPos = infile.find('?');
    if (qPos >= 0) {
      subsong = std::stoi(infile.substr(qPos + 1));
      infile = infile.substr(0, qPos);
    }
  }

  std::ifstream file(infile, std::ios::in | std::ios::binary);
  int numSamples = ::load2DX(&clef, &file, 0, verbose ? 0 : subsong + 1);
  if (!numSamples) {
    std::cerr << programName << ": unable to load bank" << std::endl;
    return 1;
  }
  if (args.hasKey("verbose")) {
    std::cerr << "Subsongs found in " << infile << ":";
    for (int i = 0; i < numSamples; i++) {
      SampleData* sample = clef.getSample(i + 1);
      if (sample) {
        std::cerr << " " << i;
      }
    }
    std::cerr << std::endl;
  }
  SampleData* sample = clef.getSample(subsong + 1);
  if (!sample) {
    std::cerr << programName << ": index " << subsong << " not in bank" << std::endl;
    return 1;
  }
  std::string outfile = args.getString("output");
  if (outfile.empty()) {
    outfile = infile + "-" + std::to_string(subsong) + ".wav";
  }
  return writeSample(&clef, sample, outfile);
}

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "verbose", "v", "", "Output additional information about input files" },
    { "output", "o", "filename", "Set the output filename (default: input filename with .wav extension)" },
    { "wma", "", "filename", "Decode a WMA file instead of playing a sequence" },
    { "mute", "m", "parts", "Silence the selected channels (gitadora only)" },
    { "solo", "s", "parts", "Only play the selected channels (gitadora only)" },
    { "preview", "p", "", "Play the preview clip instead of the sequence (pop'n only)" },
    { "subsong", "n", "index", "Play a subsong other than the first (.2dx/.ssp banks only)" },
    // TODO: save-tags
    { "", "", "input", "Path to a .1 sequence, .ssp bank, .2dx bank, or one or more .ifs files" },
  });
  std::string argError = args.parse(argc, argv);
  if (!argError.empty()) {
    std::cerr << argError << std::endl;
    return 1;
  } else if (args.hasKey("help") || argc < 2) {
    std::cout << args.usageText(argv[0]) << std::endl;
    std::cout << std::endl;
    std::cout << "For gitadora mute or solo, specify one or more channels (e.g. \"dgbk\"):" << std::endl;
    std::cout << "\td  Drums" << std::endl;
    std::cout << "\tg  Guitar" << std::endl;
    std::cout << "\tb  Bass" << std::endl;
    std::cout << "\tk  Keyboard" << std::endl;
    std::cout << "\ts  Streamed backing track" << std::endl;
    return 0;
  } else if (!args.positional().size() && !args.hasKey("wma")) {
    std::cerr << argv[0] << ": at least one input filename required" << std::endl;
    return 1;
  }

  ClefContext clef;

  if (args.hasKey("wma")) {
    return decodeWma(&clef, args.getString("wma"), args.getString("output", args.getString("wma") + ".wav"));
  }

  std::string infile = args.positional().at(0);
  BemaniFileType fileType = identifyFileType(&clef, infile);
  if (fileType == FT_invalid) {
    std::cerr << argv[0] << ": " << infile << " - unknown file type" << std::endl;
    return 1;
  }

  try {
    if (args.hasKey("verbose")) {
      // TODO: native tag format
      std::string m3uPath = TagsM3U::relativeTo(infile);
      std::ifstream tags(m3uPath);
      if (tags) {
        TagsM3U m3u(tags);
        int subsong = args.getInt("subsong");
        std::string trackName = infile;
        if (subsong) {
          trackName = infile + "?" + std::to_string(subsong);
        }
        int trackIndex = m3u.findTrack(trackName);
        if (trackIndex < 0 && !subsong) {
          trackName = infile + "?0";
        }
        if (trackIndex >= 0) {
          std::cerr << "Tags found in " << m3uPath << ":" << std::endl;
          for (const auto& iter : m3u.allTags(trackIndex)) {
            std::cerr << iter.first << " = " << iter.second << std::endl;
          }
        }
      }
    }

    if (fileType == FT_ifs) {
      return processIFS(args, clef, argv[0]);
    } else if (fileType == FT_2dx) {
      return process2dxStream(args, clef, argv[0]);
    }
    IIDXSequence seq(&clef, infile);

    SynthContext* ctx = seq.initContext();
    std::string filename = args.getString("output", seq.basePath + "wav");
    saveOutput(ctx, filename);
    return 0;
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  }
}
