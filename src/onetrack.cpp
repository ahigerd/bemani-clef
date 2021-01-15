#include "onetrack.h"
#include "utility.h"
#include <stdexcept>
#include <iostream>

// Record format:
// uint32: offset
// uint8: command
// uint8: param
// int16: value
//
// Commands:
// 0/1 = key (P1/P2)
//  param=7: play sample again after delay=value
// 2/3 = load sample (P1/P2)
//  param=key ID, sample=value
// 6 = end
// 7 = background note
//  sample=value
//
// offset = 0x7fffffff means EOF

OneTrack::OneTrack(std::istream& file)
{
  std::vector<uint64_t> keySamples[2] = {
    std::vector<uint64_t>(8),
    std::vector<uint64_t>(8),
  };
  std::vector<char> buffer(8);
  file.read(buffer.data(), 8);
  uint32_t chartLen = parseInt<uint32_t>(buffer, 4) / 8;
  file.seekg(parseInt<uint32_t>(buffer, 0));
  for (int i = 0; i < chartLen; i++) {
    if (!file.read(buffer.data(), 8)) {
      throw std::runtime_error("Unexpected EOF parsing chart");
    }
    uint32_t offset = parseInt<uint32_t>(buffer, 0);
    if (offset == 0x7FFFFFFF) {
      break;
    }
    uint8_t command = buffer[4];
    uint8_t param = buffer[5];
    uint16_t value = parseInt<uint16_t>(buffer, 6);
    switch (command) {
    case 0:
    case 1:
      {
        SampleEvent* event = new SampleEvent;
        event->timestamp = offset / 1000.0;
        event->sampleID = keySamples[command][param];
        addEvent(event);
        if (param == 7 && value) {
          event = new SampleEvent;
          event->timestamp = (offset + value) / 1000.0;
          event->sampleID = keySamples[command][param];
          addEvent(event);
        }
      }
      break;
    case 16:
      command -= 14;
    case 2:
    case 3:
      keySamples[command - 2][param] = value;
      break;
    case 4:
    case 5:
      // intentionally ignored
      break;
    case 6:
      // end event
      return;
    case 7:
      {
        SampleEvent* event = new SampleEvent;
        event->timestamp = offset / 1000.0;
        event->sampleID = value;
        addEvent(event);
      }
      break;
    default:
      //std::cerr << "unknown command: " << int(command) << std::endl;
      //throw std::runtime_error("Unknown command in chart");
      break;
    }
  }
}

