#include "codec/sampledata.h"
#include "bankloaders.h"
#include "iidxsequence.h"
#include "identify.h"
#include "ifs/ifssequence.h"
#include "ifs/ifs.h"
#include "plugin/baseplugin.h"

namespace {
struct Subsong {
  Subsong(const std::string& fullName) {
    subsong = 0;
    int qPos = fullName.find('?');
    if (qPos >= 0) {
      subsong = std::stoi(fullName.substr(qPos + 1));
      filename = fullName.substr(0, qPos);
    } else {
      filename = fullName;
    }
  }

  int subsong;
  std::string filename;
};

struct FilePtr : public Subsong {
  FilePtr(ClefContext* clef, const std::string& fullName, std::istream& fileRef)
  : Subsong(fullName) {
    if (fileRef) {
      file = &fileRef;
    } else {
      owned = clef->openFile(filename);
      file = owned.get();
    }
  }

  operator std::istream*() { return file; }

private:
  std::istream* file;
  std::unique_ptr<std::istream> owned;
};
}

struct ClefPluginInfo {
  CLEF_PLUGIN_STATIC_FIELDS

  static std::vector<std::string> getSubsongs(ClefContext* clef, const std::string& filename, std::istream& file) {
    std::vector<std::string> subsongs;
    BemaniFileType fileType = identifyFileType(clef, filename, &file);
    if (fileType == FT_2dx) {
      FilePtr fp(clef, filename, file);
      std::vector<uint64_t> ids = get2DXSampleIDs(clef, fp);
      for (uint64_t id : ids) {
        subsongs.push_back(fp.filename + "?" + std::to_string(id - 1));
      }
    }
    return subsongs;
  }

  static bool isPlayable(ClefContext* clef, const std::string& filename, std::istream& file) {
    return identifyFileType(clef, filename, &file) != FT_invalid;
  }

  static double length(ClefContext* clef, const std::string& filename, std::istream& file) {
    BemaniFileType fileType = identifyFileType(clef, filename, &file);
    if (fileType == FT_ifs) {
      IFSSequence seq(clef);
      seq.addIFS(new IFS(file));
      return seq.duration();
    } else if (fileType == FT_2dx) {
      FilePtr fp(clef, filename, file);
      return get2DXSampleLength(fp, fp.subsong + 1);
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
    BemaniFileType fileType = identifyFileType(clef, filename, &file);
    if (fileType == FT_ifs) {
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
    } else if (fileType == FT_2dx) {
      FilePtr fp(clef, filename, file);
      if (!::load2DX(clef, fp, fp.subsong)) {
        return nullptr;
      }
      SynthContext* synth = new SynthContext(clef, 44100);
      stream.reset(new StreamSequence(clef, fp.subsong + 1));
      synth->addChannel(stream->getTrack(0));
      return synth;
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
  std::unique_ptr<StreamSequence> stream;
};

const std::string ClefPluginInfo::version = "0.3.5";
const std::string ClefPluginInfo::pluginName = "bemani-clef Plugin";
const std::string ClefPluginInfo::pluginShortName = "bemani-clef";
ConstPairList ClefPluginInfo::extensions = {
  { "1", "Konami .1 sequences (*.1)" },
  { "2dx", "Konami .2dx sample banks (*.2dx)" },
  { "ssp", "Konami .ssp sample banks (*.ssp)" },
  // { "s3p", "Konami .s3p sample banks (*.s3p)" },
  { "ifs", "Konami IFS files (*.ifs)" },
};
const std::string ClefPluginInfo::about =
  "bemani-clef copyright (C) 2020-2025 Adam Higerd\n"
  "Distributed under the LGPLv2 license.";

CLEF_PLUGIN(ClefPluginInfo);
