#include "phasetrack.h"
#include "ifssequence.h"
#include "clefcontext.h"

PhaseTrack::PhaseTrack(ClefContext* ctx, const std::vector<uint64_t>& streamIDs)
{
  std::vector<SampleEvent*> events;
  double maxOffset = 0;
  for (uint64_t streamID : streamIDs) {
    SampleData* sample = ctx->getSample(streamID & ~SampleSpaces::Invert);
    sampleRate = sample->sampleRate;
    int offset = 0, prevSample = 0;
    for (int i = 0; i < sample->numSamples(); i++) {
      int s = sample->channels[0][i];
      if (s < prevSample && prevSample > 128) {
        offset = i;
        break;
      }
      prevSample = s;
    }
    SampleEvent* event = new SampleEvent;
    event->sampleID = sample->sampleID;
    event->timestamp = -offset / sample->sampleRate;
    event->duration = sample->duration();
    event->volume = (streamID & SampleSpaces::Invert) ? -1 : 1;
    if (event->timestamp < maxOffset) {
      maxOffset = event->timestamp;
    }
    events.push_back(event);
  }
  for (SampleEvent* event : events) {
    event->timestamp += -maxOffset;
    addEvent(event);
  }
}

