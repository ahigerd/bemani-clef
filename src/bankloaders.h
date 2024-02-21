#ifndef B2W_BANKLOADERS_H
#define B2W_BANKLOADERS_H

#include <fstream>
#include <stdint.h>

class ClefContext;
bool loadS3P(ClefContext* ctx, std::istream* file, uint64_t space = 0);
bool load2DX(ClefContext* ctx, std::istream* file, uint64_t space = 0);

#endif
