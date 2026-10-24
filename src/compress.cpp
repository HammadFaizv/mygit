#include "compress.h"

#include <stdexcept>
#include <vector>
#include <zlib.h>

std::string zlibCompress(const std::string &data)
{
    uLongf outLen = compressBound(data.size());
    std::vector<Bytef> out(outLen);
    if (compress(out.data(), &outLen, reinterpret_cast<const Bytef *>(data.data()), data.size()) != Z_OK)
        throw std::runtime_error("compression failed");
    return std::string(out.begin(), out.begin() + outLen);
}

std::string zlibDecompress(const std::string &data)
{
    z_stream zs{};
    if (inflateInit(&zs) != Z_OK)
        throw std::runtime_error("inflateInit failed");

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = data.size();

    std::string out;
    char buf[32768];
    int ret;
    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            inflateEnd(&zs);
            throw std::runtime_error("decompression failed (corrupt object?)");
        }
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);
    return out;
}
