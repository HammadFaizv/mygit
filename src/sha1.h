#ifndef SHA1_H
#define SHA1_H

#include <string>

// Returns the 40-character lowercase hex SHA-1 digest of `data`.
std::string sha1Hex(const std::string &data);

#endif
