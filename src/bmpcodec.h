#ifndef GD2W_BMPCODEC_H
#define GD2W_BMPCODEC_H

#include "codec/icodec.h"

class BmpCodec : public ICodec
{
public:
  BmpCodec(ClefContext* ctx);

  virtual SampleData* decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID = 0);
};

#endif
