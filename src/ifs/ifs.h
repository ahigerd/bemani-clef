#ifndef GD2W_IFS_H
#define GD2W_IFS_H

#include "clefconfig.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include "manifest.h"
#include <stdint.h>

class IFS {
public:
  static std::string pairedFile(const std::string& filename);

  IFS(std::istream& source);

  void addData(const char* buffer, ssize_t length);

  Manifest manifest;
  std::unordered_map<std::string, std::vector<uint8_t>> files;
};

#endif
