#include "ifs.h"
#include "utility.h"
#include "manifest.h"
#include "synth/synthcontext.h"
#include "ifssequence.h"
#include <fstream>
#include <sstream>
#include <exception>

struct FileNode {
  std::string name;
  uint32_t start, size;
};

std::string IFS::pairedFile(const std::string& filename)
{
  std::string fn2(filename);
  int extPos = fn2.rfind("bgm.ifs");
  if (extPos != std::string::npos) {
    fn2.replace(extPos, 3, "seq");
    return fn2;
  } else {
    extPos = fn2.rfind("seq.ifs");
    if (extPos != std::string::npos) {
      fn2.replace(extPos, 3, "bgm");
      return fn2;
    }
  }
  return std::string();
}

IFS::IFS(std::istream& source)
{
  char header[36];
  source.read(header, 36);
  if (!source.good()) {
    throw std::runtime_error("Error reading IFS header");
  }
  uint32_t magic = parseIntBE<uint32_t>(header, 0);
  uint16_t version = parseInt<uint16_t>(header, 4);
  uint16_t xorVersion = parseInt<uint16_t>(header, 6);
  if (magic != 0x6CAD8F89 || (version ^ xorVersion) != 0xFFFF) {
    throw std::runtime_error("Invalid IFS header");
  }
  uint32_t manifestEnd = parseIntBE<uint32_t>(header, 16);

  std::vector<char> manifestBuffer(manifestEnd - 36);
  source.read(manifestBuffer.data(), manifestEnd - 36);
  if (!source.good()) {
    throw std::runtime_error("IFS file truncated");
  }
  manifest = Manifest(manifestBuffer);

  std::vector<FileNode> pendingFiles;
  uint32_t maxOffset = 0;

  for (const ManifestNode& node : manifest.root.children[0].children) {
    if (node.tag == "_info_" || node.tag == "_super_") {
      continue;
    }
    std::string name;
    bool escape = false;
    for (char ch : node.tag) {
      if (escape) {
        if (ch == 'E') {
          name += '.';
        } else {
          name += ch;
        }
        escape = false;
      } else if (ch == '_') {
        escape = true;
      } else {
        name += ch;
      }
    }
    uint32_t start = parseIntBE<uint32_t>(node.data, 0);
    uint32_t size = parseIntBE<uint32_t>(node.data, 4);
    pendingFiles.push_back(FileNode{ name, start, size });
    if (start + size > maxOffset) {
      maxOffset = start + size;
    }
  }

  std::vector<char> buffer(maxOffset);
  source.read(buffer.data(), maxOffset);
  if (!source.good()) {
    throw std::runtime_error("IFS file truncated");
  }
  for (auto file : pendingFiles) {
    files[file.name] = std::vector<uint8_t>(buffer.begin() + file.start, buffer.begin() + file.start + file.size);
  }
}
