#ifndef GD2W_PHASETRACK_H
#define GD2W_PHASETRACK_H

#include "seq/itrack.h"
#include <vector>
class S2WContext;

class PhaseTrack : public BasicTrack {
public:
  PhaseTrack(S2WContext* ctx, const std::vector<uint64_t>& streamIDs);

  double sampleRate;
};

#endif
