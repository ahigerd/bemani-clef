#include "iidxsequence.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "synth/channel.h"
#include "commandargs.h"
#include "tagmap.h"
#include "asfcodec.h"
#include <fstream>
#include <iostream>
#include <iomanip>

int decodeWma(const std::string& infile, const std::string& filename) {
  AsfCodec wma;
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

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "verbose", "v", "", "Output additional information about input files" },
    { "output", "o", "filename", "Set the output filename (default: input filename with .wav extension)" },
    { "wma", "", "filename", "Decode a WMA file instead of playing a sequence" },
    // TODO: save-tags
    { "", "", "input", "Path to a .1 sequence, .s3p bank, or .2dx bank" },
  });
  std::string argError = args.parse(argc, argv);
  if (!argError.empty()) {
    std::cerr << argError << std::endl;
    return 1;
  } else if (args.hasKey("help") || argc < 2) {
    std::cout << args.usageText(argv[0]) << std::endl;
    return 0;
  } else if (args.positional().size() != 1 && !args.hasKey("wma")) {
    std::cerr << argv[0] << ": exactly one input filename required" << std::endl;
    return 1;
  }

  try {
    if (args.hasKey("wma")) {
      return decodeWma(args.getString("wma"), args.getString("output", args.getString("wma") + ".wav"));
    }
    std::string infile = args.positional().at(0);
    IIDXSequence seq(infile);

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
    return 0;
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  }
}
