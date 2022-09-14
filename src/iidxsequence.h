#ifndef B2W_IIDXSEQUENCE_H
#define B2W_IIDXSEQUENCE_H

#include "seq/isequence.h"
#include "synth/synthcontext.h"
#include "plugin/baseplugin.h"
#include "onetrack.h"
class S2WContext;

class IIDXSequence : public BaseSequence<OneTrack> {
public:
  IIDXSequence(S2WContext* ctx, const std::string& path);

  std::string basePath;

  double duration() const;

  SynthContext* initContext();

private:
  bool loadS3P();
  bool load2DX();

  std::unique_ptr<SynthContext> synth;
};

#endif
