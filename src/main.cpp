#include "ifs/ifs.h"
#include "ifs/ifssequence.h"
#include "iidxsequence.h"
#include "riffwriter.h"
#include "s2wcontext.h"
#include "synth/synthcontext.h"
#include "synth/channel.h"
#include "commandargs.h"
#include "tagmap.h"
#include "wma/asfcodec.h"
#include <fstream>
#include <iostream>
#include <iomanip>

int decodeWma(S2WContext* ctx, const std::string& infile, const std::string& filename) {
  AsfCodec wma(ctx);
  auto sample = wma.decodeFile(infile);
  RiffWriter riff(sample->sampleRate, sample->channels.size() > 1);
  std::cerr << "writing to " << filename << std::endl;
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
  std::cerr << "finished writing " << sample->channels[0].size() << " to " << filename << std::endl;
  return 0;
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

int processIFS(CommandArgs& args, S2WContext& s2w, const char* programName)
{
  IFSSequence seq(&s2w, args.hasKey("preview"));
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
    // TODO: save-tags
    { "", "", "input", "Path to one or more .1 sequences, .s3p banks, .2dx banks, or .ifs files" },
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

  S2WContext s2w;

  if (args.hasKey("wma")) {
    return decodeWma(&s2w, args.getString("wma"), args.getString("output", args.getString("wma") + ".wav"));
  }

  std::string infile = args.positional().at(0);
  bool isIFS = false;
  try {
    std::ifstream file(infile, std::ios::in | std::ios::binary);
    IFS ifs(file);
    isIFS = true;
  } catch (...) {
    // Parsing as IFS failed
  }


  try {
    if (isIFS) {
      return processIFS(args, s2w, argv[0]);
    }
    IIDXSequence seq(&s2w, infile);

    if (args.hasKey("verbose")) {
      // TODO: native tag format
      std::string m3uPath = TagsM3U::relativeTo(infile);
      std::ifstream tags(m3uPath);
      if (tags) {
        TagsM3U m3u(tags);
        int trackIndex = m3u.findTrack(infile);
        if (trackIndex >= 0) {
          std::cerr << "Tags found in " << m3uPath << ":" << std::endl;
          for (const auto& iter : m3u.allTags(trackIndex)) {
            std::cerr << iter.first << " = " << iter.second << std::endl;
          }
        }
      }
    }

    SynthContext* ctx = seq.initContext();
    std::string filename = args.getString("output", seq.basePath + "wav");
    saveOutput(ctx, filename);
    return 0;
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  }
}
