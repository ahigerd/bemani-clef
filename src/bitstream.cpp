#include "bitstream.h"
#include "utility.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

BitStream::BitStream(const BitStream::Iter8& begin, const BitStream::Iter8& end, int maxPacketSize)
: ptr(begin), end(end), packetEnd(ptr), maxPacketSize(maxPacketSize), nextPadding(0), packetSeq(0), bitOffset(0), usingReservoir(false),
  resStart(end), resEnd(end), resOffset(0), resEndOffset(0), bitsRead(0)
{
  // initializers only
}

BitStream::Pos BitStream::tell() const
{
  return Pos{ ptr, packetEnd, bitOffset };
}

void BitStream::seek(const BitStream::Pos& pos)
{
  ptr = pos.ptr;
  packetEnd = pos.packetEnd;
  bitOffset = pos.bitOffset;
}

bool BitStream::read()
{
  if (ptr >= end) {
    throw std::runtime_error("BitStream::read overflow");
  }
  if (ptr >= packetEnd) {
    startPacket();
  }

  Iter8& p = usingReservoir ? resStart : ptr;
  uint8_t& o = usingReservoir ? resOffset : bitOffset;

  bool result = (*p >> (7 - o)) & 1;
  //std::cerr << "read(1) = " << (result ? 1 : 0) << " " << (usingReservoir ? "r" : "-") << std::endl;
  o++;
  if (o == 8) {
    ++p;
    o = 0;
  }
  if (!usingReservoir) {
    bitsRead++;
  } else if (p == resEnd && o == resEndOffset) {
    //std::cerr << "end reservoir" << std::endl;
    usingReservoir = false;
  }
  return result;
}

void BitStream::skip(int count)
{
  if (count) read(count);
  return;
  if (count + bitOffset < 8) {
    bitOffset += count;
    return;
  }
  count -= 8 - bitOffset;
  ptr += 1 + (count / 8);
  bitOffset = count % 8;
}

void BitStream::skipToByte()
{
  if (bitOffset > 0) {
    ++ptr;
    bitOffset = 0;
  }
}

void BitStream::nextPacket()
{
  ptr = packetEnd;
  bitOffset = 0;
  startPacket();
}

uint64_t BitStream::peek(int count)
{
  auto pos = tell();
  uint64_t rv = read(count);
  seek(pos);
  return rv;
}

uint64_t BitStream::read(int count)
{
  int oc = count;
  uint64_t result = 0;
  while (count-- > 0) {
    result = (result << 1) | read();
  }
  return result;
#if 0
  if (count > remaining() || count < 0) {
    throw std::runtime_error("BitStream::read overflow");
  }
  int firstByte = 8 - bitOffset;
  if (firstByte > count) {
    firstByte = count;
  }
  uint64_t result = 0;
  while (firstByte > 0) {
    result = (result << 1) | read();
    --firstByte;
    --count;
  }
  count -= firstByte;
  bitOffset = (bitOffset + firstByte) % 8;
  while (count >= 8) {
    result = (result << 8) | *++ptr;
    count -= 8;
  }
  if (count > 0) {
    result = (result << count) | (*ptr & ((1 << count) - 1));
  }
  bitOffset = count;
  return result;
#endif
}

uint32_t BitStream::remaining() const {
  if (ptr >= end) {
    return 0;
  }
  return ((end - ptr) << 3) - bitOffset + (usingReservoir ? bitsReserved() : 0);
}

void BitStream::reserve(int bits) {
  // Note: resEnd + resEndOffset points to one past the last bit
  resStart = ptr;
  resOffset = bitOffset;
  if (bitOffset + bits < 8) {
    resEnd = ptr;
    resEndOffset = resOffset + bits;
    return;
  }
  bits += bitOffset;
  resEnd = ptr + (bits >> 3);
  resEndOffset = bits & 0x7;
}

void BitStream::useReserved() {
  usingReservoir = true;
}

void BitStream::flushReserved() {
  usingReservoir = false;
  resStart = resEnd;
  resOffset = resEndOffset;
}

int BitStream::bitsReserved() const {
  if (resEnd > resStart || (resEnd == resStart && resEndOffset > resOffset)) {
    return ((resEnd - resStart) << 3) - resOffset + resEndOffset;
  }
  return 0;
}

static inline uint8_t convertSize(uint8_t flags, uint8_t offset) {
  return ((flags >>= offset) & 0x3) ? 1 << (flags & 0x3) - 1 : 0;
}

static inline uint32_t parseSize(const BitStream::Iter8& pkt, int offset, uint8_t size, uint32_t defaultValue = 0) {
  return size == 1 ? parseInt<uint8_t>(pkt, offset) :
    size == 2 ? parseInt<uint16_t>(pkt, offset) :
    size == 4 ? parseInt<uint32_t>(pkt, offset) :
    defaultValue;
}

void BitStream::startPacket() {
  if (nextPadding) {
    ptr += nextPadding;
    nextPadding = 0;
  }
  //std::cerr << "BITSTREAM: new packet #" << (packetSeq++) << std::endl;
  packetStart = ptr;
  uint8_t packetFlags = *ptr++;
  if ((packetFlags & 0xe0) == 0x80) {
    // Standard ECC header present: skip it
    ptr += packetFlags & 0xf;
    //packetStart = ptr;
    packetFlags = *ptr++;
  }
  //std::cerr << "flags=" << (int)packetFlags << std::endl;
  uint8_t packetProps = *ptr++;

  uint8_t packetSizeSize = convertSize(packetFlags, 5);
  uint8_t paddingSizeSize = convertSize(packetFlags, 3);
  uint8_t sequenceTypeSize = convertSize(packetFlags, 1);
  int packetSize = parseSize(ptr, 0, packetSizeSize, maxPacketSize);

  // Ignore unused payload fields
  ptr += packetSizeSize + sequenceTypeSize;
  nextPadding = parseSize(ptr, 0, paddingSizeSize, 0);
  //std::cerr << "packetSize=" << packetSize << " " << (int)packetSizeSize << std::endl;
  //std::cerr << "paddingSize=" << nextPadding << " " << (int)paddingSizeSize << std::endl;
  ptr += paddingSizeSize + 6;
  packetEnd = packetStart + packetSize - nextPadding;

  // Ignore "replicated data"
  int repSizeSize = convertSize(packetProps, 0);
  int offSizeSize = convertSize(packetProps, 2);
  int monSizeSize = convertSize(packetProps, 4);
  int stmSizeSize = convertSize(packetProps, 6);
  // std::cerr << "sizes " << repSizeSize << " " << offSizeSize << " " << monSizeSize << " " << stmSizeSize << std::endl;
  uint32_t stream = parseSize(ptr, 0, stmSizeSize);
  ptr += stmSizeSize;
  uint32_t mon = parseSize(ptr, 0, monSizeSize);
  ptr += monSizeSize;
  uint32_t off = parseSize(ptr, 0, offSizeSize);
  ptr += offSizeSize;
  uint32_t repSize = parseSize(ptr, 0, repSizeSize);
  ptr += repSizeSize;
  // std::cerr << "stm " << stream << " " << mon << " " << off << " " << repSize << std::endl;
  ptr += repSize;
  bitOffset = 0;
  // std::cerr << "packet size: " << packetSize << " / " << (packetEnd - ptr) << std::endl;
}

uint32_t BitStream::bitsConsumed() const
{
  return bitsRead;
}

void BitStream::resetBitsConsumed()
{
  bitsRead = 0;
}
