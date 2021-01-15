#ifndef B2W_BITSTREAM_H
#define B2W_BITSTREAM_H

#include <vector>
#include <cstdint>

class BitStream {
public:
  using Iter8 = std::vector<uint8_t>::const_iterator;
  BitStream(const Iter8& begin, const Iter8& end, int maxPacketSize);

  struct Pos {
    Iter8 ptr, packetEnd;
    uint8_t bitOffset;

    int operator-(const Pos& other) const {
      return (ptr - other.ptr) * 8 - bitOffset + other.bitOffset;
    }
  };

  Pos tell() const;
  void seek(const Pos& pos);

  bool read();
  void skip(int count);
  void skipToByte();
  void nextPacket();
  uint64_t peek(int count);
  uint64_t read(int count);
  uint32_t remaining() const;
  void reserve(int bits);
  void useReserved();
  void flushReserved();
  int bitsReserved() const;
  uint32_t bitsConsumed() const;
  void resetBitsConsumed();

private:
  void startPacket();

  Iter8 ptr, end, packetStart, packetEnd;
  int maxPacketSize;
  int nextPadding;
  int packetSeq;
  uint8_t bitOffset;
  Iter8 startOffset;
  Iter8 resStart, resEnd;
  uint8_t resOffset, resEndOffset;
  uint32_t bitsRead;
  bool usingReservoir;
};

#endif
