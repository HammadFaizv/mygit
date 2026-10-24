#ifndef COMPRESS_H
#define COMPRESS_H

#include <string>

// zlib wrappers. Both throw std::runtime_error on failure.
std::string zlibCompress(const std::string &data);
std::string zlibDecompress(const std::string &data);

#endif
