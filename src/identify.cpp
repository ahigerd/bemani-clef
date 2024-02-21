#include "identify.h"

static bool isIfsFile(char header[36])
{
  uint32_t magic = parseIntBE<uint32_t>(header, 0);
  if (magic == 0x6CAD8F89) {
    // IFS magic number
    uint16_t version = parseInt<uint16_t>(header, 4);
    uint16_t xorVersion = parseInt<uint16_t>(header, 6);
    if ((version ^ xorVersion) == 0xFFFF) {
      return true;
    }
  }
  return false;
}

BemaniFileType identifyFileType(ClefContext* ctx, const std::string& filename, std::istream& file)
{
  int extPos = filename.rfind(".");
  if (extPos != std::string::npos) {
    std::string extension = filename.substr(extPos + 1);
    if (extension == "1") {
      std::string baseName = filename.substr(0, extPos);
      std::string pairName = baseName + ".s3p";
      auto pair = ctx->openFile(pairName);
      if (pair) {
        BemaniFileType pairFT = identifyFileType(ctx, pairName, *pair);
        if (pairFT == FT_s3p) {
          return FT_1;
        }
      }
      pairName = baseName + ".2dx";
      pair = ctx->openFile(pairName);
      if (pair) {
        BemaniFileType pairFT = identifyFileType(ctx, pairName, *pair);
        if (pairFT == FT_2dx) {
          return FT_1;
        }
      }
      return FT_invalid;
    }
  }
  char header[36];
  file.read(header, 36);
  if (!file.good()) {
    return FT_invalid;
  }
  if (isIfsFile(header)) {
    return FT_ifs;
  }
  uint32_t magic = parseIntBE<uint32_t>(header, 0);
  if (magic == 'S3P0') {
    return FT_s3p;
  }
  file.ignore(48);
  if (!file.read(header, 4)) {
    return FT_invalid;
  }
  uint32_t sampleOffset = parseInt<uint32_t>(header, 0);
  if (file.seekg(sampleOffset) && file.read(header, 12)) {
    if (parseIntBE<uint32_t>(header, 0) == '2DX9') {
      return FT_2dx;
    }
  }
  return FT_invalid;
}

bool isIfsFile(std::istream& file)
{
  char header[36];
  file.read(header, 36);
  bool ok = file.good();
  file.clear();
  file.seekg(0);
  return ok && isIfsFile(header);
}
