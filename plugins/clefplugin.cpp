#include "codec/sampledata.h"
#include "iidxsequence.h"
#include "identify.h"
#include "ifs/ifssequence.h"
#include "ifs/ifs.h"
#include "plugin/baseplugin.h"

struct ClefPluginInfo {
  CLEF_PLUGIN_STATIC_FIELDS

  static bool isPlayable(ClefContext* clef, const std::string& filename, std::istream& file) {
    return identifyFileType(clef, filename, file) != FT_invalid;
  }

  static double length(ClefContext* clef, const std::string& filename, std::istream& file) {
    if (isIfsFile(file)) {
      IFSSequence seq(clef);
      seq.addIFS(new IFS(file));
      return seq.duration();
    }
    IIDXSequence seq(clef, filename);
    return seq.duration();
  }

  static TagMap readTags(ClefContext* ctx, const std::string& filename, std::istream& /* unused */) {
    TagMap tagMap = TagsM3UMixin::readTags(ctx, filename);
    if (!tagMap.count("title")) {
      tagMap = TagsM3UMixin::readTags(ctx, IFS::pairedFile(filename));
    }
    return tagMap;
  }

  static int sampleRate(ClefContext*, const std::string&, std::istream& file) {
    if (isIfsFile(file)) {
      return 48000;
    } else {
      // TODO: any known 48kHz IIDX tracks?
      return 44100;
    }
  }

  SynthContext* prepare(ClefContext* clef, const std::string& filename, std::istream& file) {
    if (isIfsFile(file)) {
      iidx.reset();
      ifs.reset(new IFSSequence(clef));
      ifs->addIFS(new IFS(file));
      try {
        auto paired(clef->openFile(IFS::pairedFile(filename)));
        if (paired) {
          ifs->addIFS(new IFS(*paired));
        }
      } catch (...) {
        // no paired file, ignore
      }
      clef->purgeSamples();
      ifs->load();
      return ifs->initContext();
    }
    ifs.reset();
    iidx.reset(new IIDXSequence(clef, filename));
    return iidx->initContext();
  }

  void release() {
    ifs.reset();
    iidx.reset();
  }

  std::unique_ptr<IIDXSequence> iidx;
  std::unique_ptr<IFSSequence> ifs;
};

const std::string ClefPluginInfo::version = "0.3.4";
const std::string ClefPluginInfo::pluginName = "bemani-clef Plugin";
const std::string ClefPluginInfo::pluginShortName = "bemani-clef";
ConstPairList ClefPluginInfo::extensions = {
  { "1", "Konami .1 sequences (*.1)" },
  { "2dx", "Konami .2dx sample banks (*.2dx)" },
  { "s3p", "Konami .s3p sample banks (*.s3p)" },
  { "ifs", "Konami IFS files (*.ifs)" },
};
const std::string ClefPluginInfo::about =
  "bemani-clef copyright (C) 2020-2024 Adam Higerd\n"
  "Distributed under the LGPLv2 license.";

CLEF_PLUGIN(ClefPluginInfo);
