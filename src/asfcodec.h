#ifndef B2W_ASFCODEC_H
#define B2W_ASFCODEC_H

#include "codec/icodec.h"
#include "utility.h"

class AsfCodec : public ICodec {
public:
  AsfCodec(S2WContext* ctx);

  virtual SampleData* decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID = 0);
  static double duration(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end);
};

#endif
