#ifndef B2W_IIDXSEQUENCE_H
#define B2W_IIDXSEQUENCE_H

#include "seq/isequence.h"
#include "synth/synthcontext.h"
#include "plugin/baseplugin.h"
#include "onetrack.h"

class IIDXSequence : public BaseSequence<OneTrack> {
public:
  IIDXSequence(const std::string& path, const OpenFn& openFile = openFstream);

  std::string basePath;

  double duration() const;

  SynthContext* initContext();

private:
  bool loadS3P();
  bool load2DX();

  OpenFn openFile;
  std::unique_ptr<SynthContext> ctx;
};

#endif
