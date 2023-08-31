#include "sq3track.h"
#include "sq2track.h"
#include "utility.h"
#include "ifssequence.h"
#include <exception>

double Sq3Track::length(const uint8_t* data, int length)
{
  while (length > 21 && parseIntBE<uint32_t>(data, 0) != 'SQ3T') {
    if (parseIntBE<uint32_t>(data, 0) == 'SEQT') {
      // This is actually a SQ2 sequence with a bad filename
      return Sq2Track::length(data, length);
    }
    ++data;
    --length;
  }
  if (length < 21) {
    return 0;
  }
  int headerSize = parseInt<uint32_t>(data, 12);
  int eventCount = parseInt<uint32_t>(data, 16);
  int eventSize = parseInt<uint32_t>(data, 28);
  int chartSize = headerSize + eventCount * eventSize;
  while (chartSize > length) {
    chartSize = headerSize + --eventCount * eventSize;
  }
  if (chartSize < 16) {
    return 0;
  }
  return parseInt<uint32_t>(data, chartSize - eventSize) / 300.0;
}

Sq3Track::Sq3Track(IFSSequence* parent, const uint8_t* data, int length, uint32_t sampleSpace)
: sq2(nullptr), parent(parent), data(nullptr), sampleSpace(sampleSpace), lastPlaybackID(0)
{
  do {
    // Find the start of a chart
    while (length > 21 && parseIntBE<uint32_t>(data, 0) != 'SQ3T') {
      if (parseIntBE<uint32_t>(data, 0) == 'SEQT') {
        // This is actually a SQ2 sequence with a bad filename
        sq2.reset(new Sq2Track(parent, data, length, sampleSpace));
        maximumTimestamp = sq2->length();
        return;
      }
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
    eventSize = parseInt<uint32_t>(data, 28);
    int chartSize = headerSize + eventCount * eventSize;
    if (chartSize > length) {
      throw std::runtime_error("chart truncated");
    }
    _data = std::vector<uint8_t>(data, data + chartSize);
    this->data = &_data[0] + headerSize;
    this->end = &_data[0] + chartSize;
  } while (length > 21 && !this->data);

  if (!this->data) {
    throw std::runtime_error("no charts in sq3");
  }
  maximumTimestamp = parseInt<uint32_t>(this->end - eventSize, 0) / 300.0;
}

double Sq3Track::length() const
{
  return maximumTimestamp;
}

bool Sq3Track::isFinished() const
{
  if (sq2) {
    return sq2->isFinished();
  }
  return !holdEvent && (eventCount <= 0 || data >= end);
}

std::shared_ptr<SequenceEvent> Sq3Track::readNextEvent()
{
  if (sq2) {
    return sq2->nextEvent();
  }
  if (holdEvent) {
    auto event = holdEvent;
    holdEvent = std::shared_ptr<SequenceEvent>(nullptr);
    return event;
  }
  while (!isFinished()) {
    if (data[4] != 0x10) {
      data += eventSize;
      --eventCount;
      continue;
    }
    SampleEvent* event = new SampleEvent;
    event->timestamp = parseInt<uint32_t>(data, 0) / 300.0;
    event->sampleID = sampleSpace | parseInt<uint32_t>(data, 32);
    event->volume = data[45] / 127.0;

    // XXX: Some rips don't use the sample IDs from the va3 metadata.
    // As a workaround, try interpreting the sample ID as a filename instead.
    auto metaIter = parent->sampleData.find(event->sampleID | SampleSpaces::ByFilename);
    if (metaIter == parent->sampleData.end()) {
      // Known songs that don't have the above issue use filenames that
      // don't match a simple pattern like that, so just use the sample ID.
      metaIter = parent->sampleData.find(event->sampleID);
    }
    if (metaIter == parent->sampleData.end()) {
      // If the requested sample doesn't exist, drum va3's may define a
      // default sample based on the note ID.
      metaIter = parent->sampleData.find(data[48] | sampleSpace | SampleSpaces::ByNote);
    }

    if (metaIter != parent->sampleData.end()) {
      event->volume *= metaIter->second.volume;
      event->pan = metaIter->second.pan;
      event->sampleID = sampleSpace | metaIter->second.sampleID;
    }

    data += eventSize;
    --eventCount;

    bool hasDuration = parseInt<uint32_t>(data, 36) > 0;
    if (hasDuration) {
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

void Sq3Track::internalReset()
{
  if (sq2) {
    sq2->reset();
    return;
  }
  this->data = &_data[0] + headerSize;
  holdEvent = std::shared_ptr<SequenceEvent>();
  lastPlaybackID = 0;
}
