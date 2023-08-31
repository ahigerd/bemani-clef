#include "sq2track.h"
#include "ifssequence.h"
#include "utility.h"

double Sq2Track::length(const uint8_t* data, int length)
{
  while (length > 21 && parseIntBE<uint32_t>(data, 0) != 'SEQT') {
    ++data;
    --length;
  }
  if (length < 21) {
    return 0;
  }
  int headerSize = parseInt<uint32_t>(data, 12);
  int eventCount = parseInt<uint32_t>(data, 16);
  int chartSize = headerSize + eventCount * 16;
  while (chartSize > length) {
    chartSize = headerSize + --eventCount * 16;
  }
  if (chartSize < 16) {
    return 0;
  }
  return parseInt<uint32_t>(data, chartSize - 16) / 300.0;
}

Sq2Track::Sq2Track(IFSSequence* parent, const uint8_t* data, int length, uint32_t sampleSpace)
: parent(parent), data(nullptr), sampleSpace(sampleSpace), lastPlaybackID(0)
{
  do {
    // Find the start of a chart
    while (length > 21 && parseIntBE<uint32_t>(data, 0) != 'SEQT') {
      ++data;
      --length;
    }
    if (length < 21) {
      break;
    }
    // Metadata charts don't contain note information, skip them
    bool isMetadata = data[21];
    if (isMetadata) {
      ++data;
      --length;
      continue;
    }
    headerSize = parseInt<uint32_t>(data, 12);
    eventCount = parseInt<uint32_t>(data, 16);
    int chartSize = headerSize + eventCount * 16;
    if (chartSize > length) {
      throw std::runtime_error("chart truncated");
    }
    _data = std::vector<uint8_t>(data, data + chartSize);
    this->data = &_data[0] + headerSize;
    this->end = &_data[0] + chartSize;
  } while (length > 21 && !this->data);

  if (!this->data) {
    throw std::runtime_error("no charts in sq2");
  }
  maximumTimestamp = parseInt<uint32_t>(this->end - 16, 0) / 300.0;
}

double Sq2Track::length() const
{
  return maximumTimestamp;
}

bool Sq2Track::isFinished() const
{
  return eventCount <= 0 || data >= end;
}

std::shared_ptr<SequenceEvent> Sq2Track::readNextEvent()
{
  if (holdEvent) {
    auto event = holdEvent;
    holdEvent = std::shared_ptr<SequenceEvent>(nullptr);
    return event;
  }
  while (!isFinished()) {
    if (data[5] != 0x00 && data[5] != 0x01) {
      data += 0x10;
      --eventCount;
      continue;
    }
    uint32_t sampleID = parseInt<uint16_t>(data, 8);
    if (!sampleID) {
      data += 0x10;
      --eventCount;
      continue;
    }
    uint32_t timestamp = parseInt<uint32_t>(data, 0);
    SampleEvent* event = new SampleEvent;
    event->timestamp = timestamp / 300.0;
    event->sampleID = sampleSpace | sampleID;
    event->volume = data[12] / 127.0;
    data += 0x10;
    --eventCount;

    if (sampleSpace != SampleSpaces::Drums) {
      uint64_t killID = lastPlaybackID;
      lastPlaybackID = event->playbackID;
      if (killID) {
        holdEvent.reset(event);
        return std::shared_ptr<SequenceEvent>(new KillEvent(killID, event->timestamp));
      }
    }
    return std::shared_ptr<SequenceEvent>(event);
  }
  return nullptr;
}

void Sq2Track::internalReset()
{
  this->data = &_data[0] + headerSize;
  holdEvent = std::shared_ptr<SequenceEvent>();
  lastPlaybackID = 0;
}
