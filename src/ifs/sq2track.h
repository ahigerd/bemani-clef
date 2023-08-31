#ifndef GD2W_SQ2TRACK_H
#define GD2W_SQ2TRACK_H

#include "seq/itrack.h"
class IFSSequence;

class Sq2Track : public ITrack {
public:
  static double length(const uint8_t* data, int length);
  Sq2Track(IFSSequence* parent, const uint8_t* data, int length, uint32_t sampleSpace);

  bool isFinished() const;
  double length() const;

protected:
  std::shared_ptr<SequenceEvent> readNextEvent();
  void internalReset();

private:
  IFSSequence* parent;
  std::vector<uint8_t> _data;
  const uint8_t* data;
  const uint8_t* end;
  int headerSize;
  int eventCount;
  uint32_t sampleSpace;
  uint64_t lastPlaybackID;
  std::shared_ptr<SequenceEvent> holdEvent;
  double maximumTimestamp;
};

#endif
