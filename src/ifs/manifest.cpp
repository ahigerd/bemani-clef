#include "manifest.h"
#include "utility.h"
#include <iostream>
#include <iomanip>

std::unordered_map<char, int> sizes = {
  { 'b', 1 },
  { 'B', 1 },
  { 'h', 2 },
  { 'H', 2 },
  { 'i', 4 },
  { 'I', 4 },
  { 'q', 8 },
  { 'Q', 8 },
  { 'f', 4 },
  { 'd', 8 },
};

// derived from https://github.com/mon/kbinxml/blob/master/kbinxml/format_ids.py
struct NodeType {
  NodeType() : type('\0'), count(0), size(0) {}
  NodeType(char type, int count, std::vector<std::string> names) : type(type), count(count), names(names), name(names[0]), size(sizes[type]) {}
  char type;
  int count;
  std::vector<std::string> names;
  std::string name;
  int size;
};

NodeType nodeTypes[] = {
  NodeType(),
  NodeType('\0', 0, { "void" }),
  NodeType('b', 1, {"s8"}),
  NodeType('B', 1, {"u8"}),
  NodeType('h', 1, {"s16"}),
  NodeType('H', 1, {"u16"}),
  NodeType('i', 1, {"s32"}),
  NodeType('I', 1, {"u32"}),
  NodeType('q', 1, {"s64"}),
  NodeType('Q', 1, {"u64"}),
  NodeType('B', -1, {"bin", "binary"}),
  NodeType('B', -1, {"str", "string"}),
  NodeType('I', 1, {"ip4"}),
  NodeType('I', 1, {"time"}),
  NodeType('f', 1, {"float", "f"}),
  NodeType('d', 1, {"double", "d"}),
  NodeType('b', 2, {"2s8"}),
  NodeType('B', 2, {"2u8"}),
  NodeType('h', 2, {"2s16"}),
  NodeType('H', 2, {"2u16"}),
  NodeType('i', 2, {"2s32"}),
  NodeType('I', 2, {"2u32"}),
  NodeType('q', 2, {"2s64", "vs64"}),
  NodeType('Q', 2, {"2u64", "vu64"}),
  NodeType('f', 2, {"2f"}),
  NodeType('d', 2, {"2d", "vd"}),
  NodeType('b', 3, {"3s8"}),
  NodeType('B', 3, {"3u8"}),
  NodeType('h', 3, {"3s16"}),
  NodeType('H', 3, {"3u16"}),
  NodeType('i', 3, {"3s32"}),
  NodeType('I', 3, {"3u32"}),
  NodeType('q', 3, {"3s64"}),
  NodeType('Q', 3, {"3u64"}),
  NodeType('f', 3, {"3f"}),
  NodeType('d', 3, {"3d"}),
  NodeType('b', 4, {"4s8"}),
  NodeType('B', 4, {"4u8"}),
  NodeType('h', 4, {"4s16"}),
  NodeType('H', 4, {"4u16"}),
  NodeType('i', 4, {"4s32', 'vs32"}),
  NodeType('I', 4, {"4u32', 'vu32"}),
  NodeType('q', 4, {"4s64"}),
  NodeType('Q', 4, {"4u64"}),
  NodeType('f', 4, {"4f", "vf"}),
  NodeType('d', 4, {"4d"}),
  NodeType('\0', 0, {"attr"}),
  NodeType(),
  NodeType('b', 16, {"vs8"}),
  NodeType('B', 16, {"vu8"}),
  NodeType('h', 8, {"vs16"}),
  NodeType('H', 8, {"vu16"}),
  NodeType('b', 1, {"bool", "b"}),
  NodeType('b', 2, {"2b"}),
  NodeType('b', 3, {"3b"}),
  NodeType('b', 4, {"4b"}),
  NodeType('b', 16,{"vb"})
};

int alignOffset(int offset, int alignment = 4)
{
  if (offset % alignment) {
    return offset + alignment - (offset % alignment);
  }
  return offset;
}

std::string unpackSixbit(const std::vector<char>& buffer, int& offset)
{
  static char unpacked[] = "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
  std::string result;
  int len = uint8_t(buffer[offset++]);

  uint64_t temp = 0;
  int bits = 0;

  while (len > 0) {
    if (bits < 6) {
      temp = (temp << 8) | uint8_t(buffer[offset++]);
      bits += 8;
    }
    result += unpacked[(temp >> (bits - 6)) & 0x3f];
    bits -= 6;
    --len;
  }

  return result;
}

// adapted from https://github.com/mon/kbinxml/blob/master/kbinxml/kbinxml.py
Manifest::Manifest(const std::vector<char>& buffer)
{
  bool compressed = buffer[1] == 0x42;
  uint32_t nodeLen = parseIntBE<uint32_t>(buffer, 4);
  int pos = 8;
  int dataPos = pos + nodeLen + 4;

  std::vector<ManifestNode*> stack;
  ManifestNode* node = &root;
  stack.push_back(node);
  while (pos < nodeLen + 8) {
    if (buffer[pos] == 0) {
      ++pos;
      continue;
    }
    bool isArray = buffer[pos] & 64;
    uint8_t type = buffer[pos++] & 0xbf;

    if (type < 57) {
      NodeType fmt = nodeTypes[type];
      std::string name;
      if (compressed) {
        name = unpackSixbit(buffer, pos);
      } else {
        int len = buffer[pos] & ~64 + 1;
        name = std::string(buffer.data() + pos + 1, len);
        pos = alignOffset(pos + len + 1);
      }

      if (fmt.name == "attr") {
        int32_t len = parseIntBE<int32_t>(buffer, dataPos);
        std::string value(buffer.data() + dataPos + 4, len);
        dataPos += len + 4;
        node->attr[name] = value;
        continue;
      }

      node->children.push_back(ManifestNode());
      stack.push_back(&node->children.back());
      node = stack.back();
      node->elementSize = 0;
      node->tag = name;
      if (type == 1) {
        continue;
      }

      uint32_t varCount = fmt.count;
      uint32_t arrayCount = 1;
      if (fmt.count < 0) {
        varCount = parseIntBE<uint32_t>(buffer, dataPos);
        dataPos += 4;
        isArray = true;
      } else if (isArray) {
        arrayCount = parseIntBE<uint32_t>(buffer, dataPos) / (fmt.size * varCount);
        dataPos += 4;
      }
      uint32_t count = varCount * arrayCount;
      uint32_t size = count * fmt.size;
      if (isArray) {
        dataPos = alignOffset(dataPos);
      }
      node->data = std::vector<char>(buffer.data() + dataPos, buffer.data() + dataPos + size);
      node->elementSize = fmt.size;
      dataPos = alignOffset(dataPos + size);
    } else if (type == 190) {
      stack.pop_back();
      node = stack.back();
    } else if (type == 191) {
      break;
    } else {
      throw std::runtime_error("unknown node type");
    }
  }
}

void Manifest::dump(ManifestNode* node, int indent)
{
  if (!node) {
    node = &root.children[0];
  }
  for (int i = 0; i < indent; i++) std::cerr << " ";
  std::cerr << "<" << node->tag;
  for (const auto& attr : node->attr) {
    std::cerr << " " << attr.first << "=\"" << attr.second << "\"";
  }
  if (node->children.size() || node->data.size()) {
    std::cerr << ">" << std::endl;
    for (ManifestNode& child : node->children) {
      dump(&child, indent + 2);
    }
    if (node->data.size()) {
      for (int i = 0; i < indent + 2; i++) std::cerr << " ";
      for (char ch : node->data) std::cerr << std::hex << std::setfill('0') << std::setw(2) << (int(ch) & 0xFF) << " " << std::dec;
      std::cerr << std::endl;
    }
    for (int i = 0; i < indent; i++) std::cerr << " ";
    std::cerr << "</" << node->tag << ">" << std::endl;
  } else {
    std::cerr << " />" << std::endl;
  }
}
