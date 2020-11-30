#include "plugin/baseplugin.h"
#include "codec/sampledata.h"
#include "iidxsequence.h"

struct S2WPluginInfo : public TagsM3UMixin {
  S2WPLUGIN_STATIC_FIELDS

  static bool isPlayable(std::istream& file) {
    // Only rely on file extensions for now
    return true;
  }

  static double length(const OpenFn& openFile, const std::string& filename, std::istream& file) {
    IIDXSequence seq(filename, openFile);
    return seq.duration();
  }

  static int sampleRate(const OpenFn& openFile, const std::string& filename, std::istream& file) {
    // TODO: any known 48kHz tracks?
    return 44100;
  }

  SynthContext* prepare(const OpenFn& openFile, const std::string& filename, std::istream& file) {
    seq.reset(new IIDXSequence(filename, openFile));
    return seq->initContext();
  }

  void release() {
    seq.reset(nullptr);
  }

  std::unique_ptr<IIDXSequence> seq;
};

const std::string S2WPluginInfo::version = "0.1.0";
const std::string S2WPluginInfo::pluginName = "bemani2wav Plugin";
const std::string S2WPluginInfo::pluginShortName = "bemani2wav";
ConstPairList S2WPluginInfo::extensions = {
  { "1", "Konami .1 sequences (*.1)" },
  { "2dx", "Konami .2dx sample banks (*.2dx)" },
};
const std::string S2WPluginInfo::about =
  "bemani2wav copyright (C) 2020 Adam Higerd\n"
  "Distributed under the MIT license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
