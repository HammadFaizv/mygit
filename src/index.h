#ifndef INDEX_H
#define INDEX_H

#include <map>
#include <string>

// Staging area. One line per staged file in .mygit/index:
//     <mode> <sha> <path>
// `path` is relative to the repository root, '/'-separated, and may contain spaces
// (mode and sha are fixed tokens, so the path is simply the rest of the line).
struct IndexEntry
{
    std::string mode;
    std::string sha;
};
using Index = std::map<std::string, IndexEntry>; // path -> entry, sorted by path

// A missing index file is treated as empty. Throws std::runtime_error if malformed.
Index loadIndex();
void saveIndex(const Index &index);

// Flattens a stored tree (recursively) into path -> entry. Throws if an object is
// missing or an entry name is unsafe (empty, '.', '..', contains '/', or '.mygit').
Index indexFromTree(const std::string &treeSha);

// Builds nested tree objects from the staged paths, stores them, returns the root SHA.
std::string writeTreeFromIndex(const Index &index);

#endif
