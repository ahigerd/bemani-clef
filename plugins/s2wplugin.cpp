#include "plugin/baseplugin.h"
#include "codec/sampledata.h"
#include "iidxsequence.h"

struct S2WPluginInfo : public TagsM3UMixin {
  S2WPLUGIN_STATIC_FIELDS

  static bool isPlayable(std::istream& file) {
    // Only rely on file extensions for now
    return true;
  }

  static double length(S2WContext* s2w, const std::string& filename, std::istream& file) {
    IIDXSequence seq(s2w, filename);
    return seq.duration();
  }

  static int sampleRate(S2WContext* s2w, const std::string& filename, std::istream& file) {
    // TODO: any known 48kHz tracks?
    return 44100;
  }

  SynthContext* prepare(S2WContext* s2w, const std::string& filename, std::istream& file) {
    seq.reset(new IIDXSequence(s2w, filename ));
    return seq->initContext();
  }

  void release() {
    seq.reset(nullptr);
  }

  std::unique_ptr<IIDXSequence> seq;
};

const std::string S2WPluginInfo::version = "0.2.3";
const std::string S2WPluginInfo::pluginName = "bemani2wav Plugin";
const std::string S2WPluginInfo::pluginShortName = "bemani2wav";
ConstPairList S2WPluginInfo::extensions = {
  { "1", "Konami .1 sequences (*.1)" },
  { "2dx", "Konami .2dx sample banks (*.2dx)" },
  { "s3p", "Konami .s3p sample banks (*.s3p)" },
};
const std::string S2WPluginInfo::about =
  "bemani2wav copyright (C) 2020-2022 Adam Higerd\n"
  "Distributed under the LGPLv2 license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
