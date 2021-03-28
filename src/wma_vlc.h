/*
 * WMA decoder copyright (c) 2020 Adam Higerd
 * FFmpeg copyright (c) 2000-2004 The FFmpeg Project, Fabrice Bellard,
 *     Michael Niedermayer <michaelni@gmx.at>
 *
 * bemani2wav is free software. Most of its source code is licensed
 * under the MIT license, but files with this notice are derived from
 * FFmpeg. As a result, these files and any binary distribution compiled
 * from them may be redistributed and/or modified under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to:
 *   Free Software Foundation, Inc.
 *   51 Franklin Street, Fifth Floor
 *   Boston MA  02110-1301
 *   USA
 */

struct VLCode {
  VLCode() {}
  VLCode(uint8_t bits, uint32_t code, uint16_t symbol, int8_t order)
  : bits(bits), order(order), symbol(symbol), code(code) {}

  uint8_t bits;
  int8_t order;
  uint16_t symbol;
  uint32_t code;

  bool operator<(const VLCode& other) const {
    if (order == other.order) {
      return (code >> 1) < (other.code >> 1);
    }
    return order < other.order;
  }
};

struct VLC {
  static VLC* get(int tableID) {
    if (tableID < 0) {
      static VLC expVlc;
      return &expVlc;
    }
    if (!cache.count(tableID)) {
      cache.emplace(tableID, tableID);
    }
    return &cache.at(tableID);
  }

  VLC(VLC&& other) = default;
  VLC& operator=(VLC&& other) = default;

  VLC() : bits(8)
  {
    // initializers only
    initCodes(121, ff_aac_scalefactor_code, ff_aac_scalefactor_bits);
  }

  VLC(int tableID)
  : runTable({ 0, 0 }),
    levelTable({ 0, 0 }),
    bits(9)
  {
    const CoefVLCTable* coefVlc = &coef_vlcs[tableID];
    initCodes(coefVlc->n, coefVlc->huffcodes, coefVlc->huffbits);

    for (int i = 2, k = 0; k < coefVlc->max_level; k++) {
      for (int j = 0; j < coefVlc->levels[k]; i++, j++) {
        runTable.push_back(j);
        levelTable.push_back(k + 1);
      }
    }
  }

  int extractFrom(BitStream& bitstream) const {
    static int outcount = 0;
    uint32_t prefix = 0;
    uint32_t padding = 0x0002;
    for (int k = 0; k < 31; k++) {
      prefix = (prefix << 1) | bitstream.read();
      auto iter = lookup.find(prefix | padding);
      if (iter != lookup.end()) {
        return iter->second;
      }
      padding <<= 1;
    }
    throw std::runtime_error("invalid bitstream");
  }

  std::unordered_map<uint32_t, uint32_t> lookup;
  std::vector<uint16_t> runTable;
  std::vector<float> levelTable;
  int bits;

private:
  void initCodes(int n, const uint32_t* code, const uint8_t* bits) {
    uint32_t padding;
    for (uint16_t i = 0; i < n; i++) {
      padding = 1 << bits[i];
      lookup[code[i] | padding] = i;
    }
  }

  static std::unordered_map<uint64_t, VLC> cache;
};
std::unordered_map<uint64_t, VLC> VLC::cache;
