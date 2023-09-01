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

OneTrack::OneTrack(std::istream& file, bool popn)
: BasicTrack()
{
  std::vector<uint64_t> keySamples[2] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  };
  std::vector<char> buffer(17);
  file.read(buffer.data(), 17);
  int eventSize = 8;
  if (popn && buffer[17] == 0x45) {
    // extended record format
    eventSize = 12;
  }
  uint32_t chartLen = popn ? 0x7FFFFFFF : parseInt<uint32_t>(buffer, 4) / eventSize;
  file.seekg(parseInt<uint32_t>(buffer, 0));
  uint32_t offset;
  uint8_t command, param;
  uint16_t value;
  int32_t filepos = -8;
  for (int i = 0; i < chartLen; i++) {
    filepos += eventSize;
    if (!file.read(buffer.data(), eventSize)) {
      if (popn) {
        return;
      } else {
        throw std::runtime_error("Unexpected EOF parsing chart");
      }
    }
    offset = parseInt<uint32_t>(buffer, 0);
    if (offset == 0x7FFFFFFF) {
      break;
    }
    if (popn) {
      command = buffer[5];
      if (command == 1) {
        command = 0;
      }
      if (command == 2 || command == 7) {
        value = parseInt<uint16_t>(buffer, 6);
        param = value >> 12;
        value &= 0x0FFF;
      } else {
        param = buffer[6];
        value = uint8_t(buffer[7]);
      }
    } else {
      command = buffer[4];
      param = buffer[5];
      value = parseInt<uint16_t>(buffer, 6);
    }
    //std::cerr << std::hex << filepos << ":\t" << std::dec << offset << "\t" << int(command) << "\t" << int(param) << "\t" << value << "\t" << std::hex << value << std::dec << std::endl;
    switch (command) {
    case 0:
    case 1:
      {
        SampleEvent* event = new SampleEvent;
        event->timestamp = offset / 1000.0;
        event->sampleID = keySamples[command][param];
        //std::cerr << "ks " << int(param) << " = " << event->sampleID << std::endl;
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
    case 3:
      if (popn) {
        SampleEvent* event = new SampleEvent;
        event->timestamp = offset / 1000.0;
        event->sampleID = 0x10001;
        addEvent(event);
        break;
      }
    case 2:
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
      std::cerr << "unknown command: " << int(command) << std::endl;
      //throw std::runtime_error("Unknown command in chart");
      break;
    }
  }
}

