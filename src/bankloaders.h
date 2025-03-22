#ifndef B2W_BANKLOADERS_H
#define B2W_BANKLOADERS_H

#include <fstream>
#include <vector>
#include <stdint.h>

class ClefContext;
int loadS3P(ClefContext* ctx, std::istream* file, uint64_t space = 0);
int load2DX(ClefContext* ctx, std::istream* file, uint64_t space = 0, uint64_t onlySample = 0);
std::vector<uint64_t> get2DXSampleIDs(ClefContext* ctx, std::istream* file, uint64_t space = 0);
double get2DXSampleLength(std::istream* file, uint64_t sampleID);

#endif
