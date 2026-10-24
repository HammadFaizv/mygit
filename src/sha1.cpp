#include "sha1.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

std::string sha1Hex(const std::string &data)
{
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    // Pad: 0x80, zeros up to 56 mod 64, then 64-bit big-endian bit length.
    std::vector<uint8_t> msg(data.begin(), data.end());
    uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56)
        msg.push_back(0);
    for (int i = 7; i >= 0; --i)
        msg.push_back(static_cast<uint8_t>(bitLen >> (i * 8)));

    for (size_t off = 0; off < msg.size(); off += 64)
    {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = (msg[off + 4 * i] << 24) | (msg[off + 4 * i + 1] << 16) |
                   (msg[off + 4 * i + 2] << 8) | msg[off + 4 * i + 3];
        for (int i = 16; i < 80; ++i)
            w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i)
        {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d);          k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                   k = 0xCA62C1D6; }
            uint32_t temp = rotl(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl(b, 30); b = a; a = temp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }

    char buf[41];
    for (int i = 0; i < 5; ++i)
        snprintf(buf + i * 8, 9, "%08x", h[i]);
    return std::string(buf, 40);
}
