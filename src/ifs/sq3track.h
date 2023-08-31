#ifndef GD2W_SQ3TRACK_H
#define GD2W_SQ3TRACK_H

#include "seq/itrack.h"
#include <memory>
class IFSSequence;
class Sq2Track;

class Sq3Track : public ITrack {
public:
  static double length(const uint8_t* data, int length);
  Sq3Track(IFSSequence* parent, const uint8_t* data, int length, uint32_t sampleSpace);

  bool isFinished() const;
  double length() const;

protected:
  std::shared_ptr<SequenceEvent> readNextEvent();
  void internalReset();

private:
  std::unique_ptr<Sq2Track> sq2;
  IFSSequence* parent;
  std::vector<uint8_t> _data;
  const uint8_t* data;
  const uint8_t* end;
  int headerSize;
  int eventSize;
  int eventCount;
  uint32_t sampleSpace;
  uint64_t lastPlaybackID;
  std::shared_ptr<SequenceEvent> holdEvent;
  double maximumTimestamp;
};

#endif
