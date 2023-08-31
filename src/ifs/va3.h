#ifndef GD2W_VA3_H
#define GD2W_VA3_H

#include <cstdint>
#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include <string>
class IFS;

class VA3 {
public:
  VA3(const IFS* ifs, const std::string& filename);

  struct Metadata {
    uint32_t offset;
    uint32_t size;
    uint32_t loopStart;
    uint32_t loopEnd;
    uint16_t sampleID;
    uint8_t channels;
    uint8_t sampleBits;
    double sampleRate;
    double volume;
    double pan;
  };

  int version;
  std::map<std::string, Metadata> files;
  std::pair<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::const_iterator> get(const std::string& filename) const;
  std::unordered_map<uint64_t, uint64_t> defaultDrums;

private:
  const IFS* ifs;
  std::string filename;
};

#endif
