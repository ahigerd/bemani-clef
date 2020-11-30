#include "iidxsequence.h"
#include "riffwriter.h"
#include "synth/synthcontext.h"
#include "synth/channel.h"
#include "commandargs.h"
#include "tagmap.h"
#include <fstream>
#include <iostream>
#include <iomanip>

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "verbose", "v", "", "Output additional information about input files" },
    { "output", "o", "filename", "Set the output filename (default: input filename with .wav extension)" },
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
  } else if (args.positional().size() != 1) {
    std::cerr << argv[0] << ": exactly one input filename required" << std::endl;
    return 1;
  }

  try {
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
