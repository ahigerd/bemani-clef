#ifndef B2W_BANKLOADERS_H
#define B2W_BANKLOADERS_H

#include <fstream>

class S2WContext;
bool loadS3P(S2WContext* ctx, std::istream* file, uint64_t space = 0);
bool load2DX(S2WContext* ctx, std::istream* file, uint64_t space = 0);

#endif
