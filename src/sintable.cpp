#include "sintable.h"
#include <cmath>
#include <unordered_map>
#include <iostream>

SinTable* SinTable::get(int resolution) {
  static std::unordered_map<int, SinTable> cache;
  if (!cache.count(resolution)) {
    cache.emplace(resolution, resolution);
  }
  return &cache.at(resolution);
}

SinTable::SinTable(int resolution)
: floatTable(resolution, 0.0f), intTable(resolution, 0) {
  float step = M_PI / 2.0 / resolution;
  int j = resolution - 1;
  for (int i = 0; i < resolution; i++, j--) {
    floatTable[i] = std::sin(step * j);
    intTable[i] = 0x8000 * std::sin(step * i);
  }
}
