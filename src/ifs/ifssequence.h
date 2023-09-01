#ifndef GD2W_IFSSEQUENCE_H
#define GD2W_IFSSEQUENCE_H

#include "seq/isequence.h"
#include "codec/sampledata.h"
#include "va3.h"
#include <unordered_map>
class IFS;
class SampleData;
class SynthContext;

namespace SampleSpaces {
  enum : uint64_t {
    Drums      = 0x010000,
    Guitar     = 0x020000,
    Bass       = 0x040000,
    Keyboard   = 0x080000,
    Backing    = 0x100000,
    ByFilename = 0x200000,
    ByNote     = 0x400000,
    Invert     = 0x800000,
  };
};

class IFSSequence : public BaseSequence<ITrack> {
public:
  static uint64_t stringToSpaces(const std::string& channels);
  IFSSequence(S2WContext* ctx, bool usePreview = false);

  double sampleRate;
  std::unordered_map<uint64_t, VA3::Metadata> sampleData;

  void addIFS(IFS* ifs);
  void load();
  double duration() const;
  void setMutes(const std::string& channels);
  void setSolo(const std::string& channels);

  SynthContext* initContext();

private:
  void usePhasedStreams(const std::unordered_map<uint64_t, std::string>& streams);

  uint64_t mute;
  bool usePreview;
  std::vector<std::unique_ptr<IFS>> files;
  std::unique_ptr<SynthContext> ctx;
};

#endif
