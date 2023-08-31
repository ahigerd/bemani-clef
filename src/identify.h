#ifndef B2W_IDENTIFY_H
#define B2W_IDENTIFY_H

#include <iostream>
#include <string>
#include "s2wcontext.h"

enum BemaniFileType {
  FT_invalid,
  FT_2dx,
  FT_s3p,
  FT_1,
  FT_ifs,
};

BemaniFileType identifyFileType(S2WContext* ctx, const std::string& filename, std::istream& file);
bool isIfsFile(std::istream& file);

#endif
