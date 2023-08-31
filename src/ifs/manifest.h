#ifndef GD2W_MANIFEST_H
#define GD2W_MANIFEST_H

#include <string>
#include <vector>
#include <unordered_map>

struct ManifestNode {
  std::string tag;
  std::vector<char> data;
  int elementSize;
  std::unordered_map<std::string, std::string> attr;
  std::vector<ManifestNode> children;
};

class Manifest {
public:
  Manifest() = default;
  Manifest(const std::vector<char>& buffer);
  Manifest(const Manifest& other) = default;
  Manifest& operator=(const Manifest& other) = default;

  ManifestNode root;

  void dump(ManifestNode* node = nullptr, int indent = 0);
};

#endif
