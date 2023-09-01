#ifndef B2W_ONETRACK_H
#define B2W_ONETRACK_H

#include "seq/itrack.h"
#include <iostream>
#include <memory>

class OneTrack : public BasicTrack {
public:
  OneTrack(std::istream& file, bool popn = false);
};

#endif
