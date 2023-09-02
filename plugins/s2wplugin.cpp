#include "codec/sampledata.h"
#include "iidxsequence.h"
#include "identify.h"
#include "ifs/ifssequence.h"
#include "ifs/ifs.h"
#include "plugin/baseplugin.h"

struct S2WPluginInfo {
  S2WPLUGIN_STATIC_FIELDS

  static bool isPlayable(S2WContext* s2w, const std::string& filename, std::istream& file) {
    return identifyFileType(s2w, filename, file) != FT_invalid;
  }

  static double length(S2WContext* s2w, const std::string& filename, std::istream& file) {
    if (isIfsFile(file)) {
      IFSSequence seq(s2w);
      seq.addIFS(new IFS(file));
      return seq.duration();
    }
    IIDXSequence seq(s2w, filename);
    return seq.duration();
  }

  static TagMap readTags(S2WContext* ctx, const std::string& filename, std::istream& /* unused */) {
    TagMap tagMap = TagsM3UMixin::readTags(ctx, filename);
    if (!tagMap.count("title")) {
      tagMap = TagsM3UMixin::readTags(ctx, IFS::pairedFile(filename));
    }
    return tagMap;
  }

  static int sampleRate(S2WContext*, const std::string&, std::istream& file) {
    if (isIfsFile(file)) {
      return 48000;
    } else {
      // TODO: any known 48kHz IIDX tracks?
      return 44100;
    }
  }

  SynthContext* prepare(S2WContext* s2w, const std::string& filename, std::istream& file) {
    if (isIfsFile(file)) {
      iidx.reset();
      ifs.reset(new IFSSequence(s2w));
      ifs->addIFS(new IFS(file));
      try {
        auto paired(s2w->openFile(IFS::pairedFile(filename)));
        if (paired) {
          ifs->addIFS(new IFS(*paired));
        }
      } catch (...) {
        // no paired file, ignore
      }
      s2w->purgeSamples();
      ifs->load();
      return ifs->initContext();
    }
    ifs.reset();
    iidx.reset(new IIDXSequence(s2w, filename));
    return iidx->initContext();
  }

  void release() {
    ifs.reset();
    iidx.reset();
  }

  std::unique_ptr<IIDXSequence> iidx;
  std::unique_ptr<IFSSequence> ifs;
};

const std::string S2WPluginInfo::version = "0.3.3";
const std::string S2WPluginInfo::pluginName = "bemani2wav Plugin";
const std::string S2WPluginInfo::pluginShortName = "bemani2wav";
ConstPairList S2WPluginInfo::extensions = {
  { "1", "Konami .1 sequences (*.1)" },
  { "2dx", "Konami .2dx sample banks (*.2dx)" },
  { "s3p", "Konami .s3p sample banks (*.s3p)" },
  { "ifs", "Konami IFS files (*.ifs)" },
};
const std::string S2WPluginInfo::about =
  "bemani2wav copyright (C) 2020-2023 Adam Higerd\n"
  "Distributed under the LGPLv2 license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
