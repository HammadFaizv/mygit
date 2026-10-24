#ifndef TREE_H
#define TREE_H

#include <filesystem>
#include <string>
#include <vector>

// One line of a tree object: "<mode> <name>\0<20-byte raw sha>".
struct TreeEntry
{
    std::string mode; // "100644" (file), "100755" (executable), "40000" (directory)
    std::string name;
    std::string sha; // 40-char hex

    bool isDir() const { return mode == "40000"; }
};

// True if `s` is exactly 40 hex characters.
bool isValidSha(const std::string &s);

// Conversions between 40-char hex and the 20-byte raw form stored in trees.
std::string hexToRaw(const std::string &hex);
std::string rawToHex(const std::string &raw);

// Decodes the content of a tree object. Throws std::runtime_error if malformed.
std::vector<TreeEntry> parseTree(const std::string &content);

// Mode string for a file on disk: "100755" if owner-executable, else "100644".
std::string modeForFile(const std::filesystem::file_status &st);

// Sorts `entries` in Git order (directories compare as if named "<name>/"),
// stores them as a tree object and returns its SHA. May be empty.
std::string storeTree(std::vector<TreeEntry> entries);

// Prints a tree's entries: "<mode> <type> <sha>\t<name>", or only the names.
void printTreeEntries(const std::string &treeContent, bool nameOnly);

// Recursively stores the contents of `dir` as blob/tree objects (skipping .mygit
// and anything that is not a regular file or directory). Stores the tree for
// `dir` and returns true with its SHA in `shaOut`. Directories with no
// entries are skipped (returns false) unless `isRoot` is set.
bool writeTreeFromDir(const std::filesystem::path &dir, bool isRoot, std::string &shaOut);

#endif
