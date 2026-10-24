#ifndef REPO_H
#define REPO_H

#include <string>

// Layout of the repository:
//   .mygit/objects/<2-hex>/<38-hex>   compressed objects
//   .mygit/index                      staging area
//   .mygit/HEAD                       SHA of the current commit (empty if none)
const std::string MYGIT_DIR = ".mygit";
const std::string OBJECTS_DIR = MYGIT_DIR + "/objects";
const std::string INDEX_FILE = MYGIT_DIR + "/index";
const std::string HEAD_FILE = MYGIT_DIR + "/HEAD";

bool repoExists();

// Reads / writes a whole file. readFile throws std::runtime_error on failure.
std::string readFile(const std::string &path);
void writeFile(const std::string &path, const std::string &content);

// HEAD holds a commit SHA. readHead returns "" if there is no commit yet.
std::string readHead();
void writeHead(const std::string &sha);

// Stores "<type> <size>\0<content>" compressed under its SHA-1; returns the SHA.
// If `write` is false, only the SHA is computed.
std::string storeObject(const std::string &type, const std::string &content, bool write = true);

// Loads and decompresses an object. Splits the header into type and content.
// Throws std::runtime_error if the object does not exist.
void loadObject(const std::string &sha, std::string &type, std::string &content);

#endif
