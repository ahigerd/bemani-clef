#include "bmpcodec.h"
#include "utility.h"
#include "codec/adpcmcodec.h"

BmpCodec::BmpCodec(ClefContext* ctx)
: ICodec(ctx)
{
  // initializers only
}

SampleData* BmpCodec::decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID)
{
  int channels = start[16];
  int32_t sampleRate = parseIntBE<int32_t>(start, 20);
  AdpcmCodec adpcm(context(), AdpcmCodec::OKI4s, channels == 2 ? -1 : 0);
  SampleData* sample = adpcm.decodeRange(start + 32, end, sampleID);
  sample->sampleRate = sampleRate;
  return sample;
}
