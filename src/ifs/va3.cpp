#include "va3.h"
#include "ifs.h"
#include "utility.h"
#include <iostream>

VA3::VA3(const IFS* ifs, const std::string& filename) : ifs(ifs), filename(filename)
{
  const std::vector<uint8_t>& file = ifs->files.at(this->filename);
  uint32_t magic = parseIntBE<uint32_t>(file, 0);
  uint32_t version = parseIntBE<uint32_t>(file, 4);
  uint32_t entries = parseInt<uint32_t>(file, 8);
  uint32_t gdxSize = parseInt<uint32_t>(file, 12);
  uint32_t gdxStart = parseInt<uint32_t>(file, 16);
  uint32_t entryStart = parseInt<uint32_t>(file, 20);
  uint32_t dataStart = parseInt<uint32_t>(file, 24);

  uint32_t gdxMagic = parseIntBE<uint32_t>(file, gdxStart);
  uint32_t numDefaults = gdxMagic == 'GDXG' ? 9 : 6;
  for (int i = 0; i < numDefaults; i++) {
    defaultDrums[i] = parseInt<uint16_t>(file, gdxStart + 4 + 2 * i);
  }

  uint32_t pos = entryStart;
  for (int i = 0; i < entries; i++, pos += 0x40) {
    Metadata meta;
    meta.offset = parseInt<uint32_t>(file, pos) + dataStart;
    meta.size = parseInt<uint32_t>(file, pos + 4);
    meta.channels = parseInt<uint16_t>(file, pos + 8);
    meta.sampleBits = parseInt<uint16_t>(file, pos + 10);
    meta.sampleRate = parseInt<uint32_t>(file, pos + 12);
    // TODO: loopStart, loopEnd
    meta.volume = file[pos + 24] / 127.0;
    meta.pan = file[pos + 25] / 128.0;
    meta.sampleID = parseInt<uint16_t>(file, pos + 26);
    int filenameEnd = pos + 32;
    while (filenameEnd < pos + 64 && file[filenameEnd] != '\0') {
      filenameEnd++;
    }
    std::string filename((char*)(file.data() + pos + 32), filenameEnd - pos - 32);
    files[filename] = meta;
  }
}

std::pair<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::const_iterator> VA3::get(const std::string& filename) const
{
  const Metadata& metadata = files.at(filename);
  const std::vector<uint8_t>& file = ifs->files.at(this->filename);
  auto start = file.begin() + metadata.offset;
  auto end = start + metadata.size;
  if (start > file.end()) {
    start = file.end();
    end = file.end();
  } else if (end > file.end()) {
    end = file.end();
  }
  return std::make_pair(start, end);
}
