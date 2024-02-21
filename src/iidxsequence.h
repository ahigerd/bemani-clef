#ifndef B2W_IIDXSEQUENCE_H
#define B2W_IIDXSEQUENCE_H

#include "seq/isequence.h"
#include "synth/synthcontext.h"
#include "plugin/baseplugin.h"
#include "onetrack.h"
class ClefContext;

class IIDXSequence : public BaseSequence<OneTrack> {
public:
  IIDXSequence(ClefContext* ctx, const std::string& path);

  std::string basePath;

  double duration() const;

  SynthContext* initContext();

private:
  bool loadS3P();
  bool load2DX();

  std::unique_ptr<SynthContext> synth;
};

#endif
