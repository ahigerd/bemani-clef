#include "asfcodec.h"
#include "wmacodec.h"

using AsfGuid = std::array<uint8_t, 16>;
static const AsfGuid fileProps  { 0xa1, 0xdc, 0xab, 0x8c, 0x47, 0xa9, 0xcf, 0x11, 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 };
static const AsfGuid streamProps{ 0x91, 0x07, 0xdc, 0xb7, 0xb7, 0xa9, 0xcf, 0x11, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 };
static const AsfGuid asfData    { 0x36, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c };

Iter8 findGuid(const AsfGuid& guid, Iter8 start, Iter8 end)
{
  auto iter = guid.begin();
  auto gEnd = guid.end();
  while (start < end && iter < gEnd) {
    if (*start++ == *iter) {
      ++iter;
    } else {
      iter = guid.begin();
    }
  }
  return start;
}

std::pair<Iter8, Iter8> findWma(Iter8 start, Iter8 end) {
  start = findGuid(asfData, start, end);
  if (start + 58 > end) {
    return std::make_pair(end, end);
  }
  uint64_t size = parseInt<uint64_t>(start, 0) - 24;
  Iter8 wmaEnd = start + size;
  start += 34; // skip size, file ID, packet count, reserved
  if (start > end) {
    return std::make_pair(end, end);
  }
  return std::make_pair(start, wmaEnd);
}

SampleData* AsfCodec::decodeRange(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, uint64_t sampleID)
{
  Iter8 propStart = findGuid(streamProps, start, end);
  if (propStart == end) {
    std::cerr << "No stream props" << std::endl;
    return nullptr;
  }
  int propSize = parseInt<uint32_t>(propStart, 48);
  Iter8 propEnd = propStart + 62 + propSize;

  Iter8 filePropStart = findGuid(fileProps, start, end);
  uint32_t maxPacketSize = parseInt<uint32_t>(filePropStart, 80);

  auto wma = findWma(propEnd, end);
  if (wma.first == end) {
    std::cerr << "No WMA data" << std::endl;
    return nullptr;
  }

  WaveFormatEx fmt(propStart + 62, propEnd);
  if (fmt.format != 0x0161) {
    std::cerr << "Not WMAv2" << std::endl;
    return nullptr;
  }

  std::unique_ptr<WmaCodec> wmaCodec;
  try {
    wmaCodec.reset(new WmaCodec(fmt, maxPacketSize));
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return nullptr;
  }
  return wmaCodec->decodeRange(wma.first, wma.second);
}

double AsfCodec::duration(Iter8 start, Iter8 end)
{
  Iter8 iter = findGuid(fileProps, start, end);
  if (iter != end) {
    return parseInt<uint64_t>(iter, 48) / 10000000.0;
  }
  return -1;
}
