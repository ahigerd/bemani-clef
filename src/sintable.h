#ifndef B2W_SINTABLE_H
#define B2W_SINTABLE_H

#include <vector>
#include <cstdint>

struct SinTable {
  static SinTable* get(int resolution);

  SinTable(int resolution);

  inline float floatOut(int index) const { return floatTable[index]; }
  inline std::int16_t intIn(int index) const { return intTable[index]; }

private:
  std::vector<float> floatTable;
  std::vector<std::int16_t> intTable;
};

#endif
