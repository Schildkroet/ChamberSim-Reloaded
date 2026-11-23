/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zipfile.h"

#include <array>
#include <cstdint>

namespace csr
{

namespace
{

// ---- inflate (RFC 1951), after the structure of zlib's "puff" ------------------------------------------------------------------------

constexpr int kMaxBits = 15;

struct Huffman
{
    std::array<int, kMaxBits + 1> count{};
    std::array<int, 288> symbol{};

    // From the code lengths of `n` symbols; false for an over-subscribed set (an incomplete one is allowed for the distance code).
    bool build(const int *lengths, int n)
    {
        count.fill(0);
        for (int i = 0; i < n; ++i) ++count[lengths[i]];
        int left = 1;
        for (int len = 1; len <= kMaxBits; ++len)
        {
            left = (left << 1) - count[len];
            if (left < 0) return false;
        }
        std::array<int, kMaxBits + 2> offsets{};
        for (int len = 1; len <= kMaxBits; ++len) offsets[len + 1] = offsets[len] + count[len];
        for (int i = 0; i < n; ++i)
            if (lengths[i] != 0) symbol[offsets[lengths[i]]++] = i;
        return true;
    }
};

class Inflater
{
public:
    Inflater(const unsigned char *in, qint64 size, qint64 maxOut)
        : m_in(in)
        , m_size(size)
        , m_maxOut(maxOut)
    {
    }

    std::optional<QByteArray> run()
    {
        bool last = false;
        while (!last)
        {
            const int l = bits(1), type = bits(2);
            last = l != 0;
            if (m_error) return std::nullopt;
            bool ok = false;
            if (type == 0) ok = stored();
            else if (type == 1) ok = fixedBlock();
            else if (type == 2) ok = dynamicBlock();
            if (!ok || m_error) return std::nullopt;
        }
        return m_out;
    }

private:
    int bits(int need)
    {
        std::uint32_t value = m_bitBuffer;
        while (m_bitCount < need)
        {
            if (m_pos >= m_size)
            {
                m_error = true;
                return 0;
            }
            value |= static_cast<std::uint32_t>(m_in[m_pos++]) << m_bitCount;
            m_bitCount += 8;
        }
        m_bitBuffer = value >> need;
        m_bitCount -= need;
        return static_cast<int>(value & ((1u << need) - 1));
    }

    int decode(const Huffman &h)
    {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len <= kMaxBits; ++len)
        {
            code |= bits(1);
            if (m_error) return -1;
            const int count = h.count[len];
            if (code - count < first) return h.symbol[index + (code - first)];
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }

    bool stored()
    {
        m_bitBuffer = 0;
        m_bitCount = 0;
        if (m_pos + 4 > m_size) return false;
        const int len = m_in[m_pos] | (m_in[m_pos + 1] << 8), inv = m_in[m_pos + 2] | (m_in[m_pos + 3] << 8);
        m_pos += 4;
        if (len != (~inv & 0xffff) || m_pos + len > m_size || m_out.size() + len > m_maxOut) return false;
        m_out.append(reinterpret_cast<const char *>(m_in + m_pos), len);
        m_pos += len;
        return true;
    }

    bool codes(const Huffman &lit, const Huffman &dist)
    {
        static constexpr std::array<int, 29> lengthBase{3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                                        31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
        static constexpr std::array<int, 29> lengthExtra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                                         2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
        static constexpr std::array<int, 30> distBase{1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                                                      33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                                                      1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
        static constexpr std::array<int, 30> distExtra{0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                                       6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
        for (;;)
        {
            const int sym = decode(lit);
            if (sym < 0) return false;
            if (sym < 256)
            {
                if (m_out.size() >= m_maxOut) return false;
                m_out.append(static_cast<char>(sym));
            }
            else if (sym == 256) return true;
            else
            {
                const int li = sym - 257;
                if (li >= 29) return false;
                const int len = lengthBase[li] + bits(lengthExtra[li]);
                const int ds = decode(dist);
                if (ds < 0 || ds >= 30) return false;
                const qint64 d = distBase[ds] + bits(distExtra[ds]);
                if (m_error || d > m_out.size() || m_out.size() + len > m_maxOut) return false;
                for (int i = 0; i < len; ++i) m_out.append(m_out[m_out.size() - d]);
            }
        }
    }

    bool fixedBlock()
    {
        int lengths[288];
        for (int i = 0; i < 144; ++i) lengths[i] = 8;
        for (int i = 144; i < 256; ++i) lengths[i] = 9;
        for (int i = 256; i < 280; ++i) lengths[i] = 7;
        for (int i = 280; i < 288; ++i) lengths[i] = 8;
        Huffman lit, dist;
        lit.build(lengths, 288);
        int dl[30];
        for (int &l : dl) l = 5;
        dist.build(dl, 30);
        return codes(lit, dist);
    }

    bool dynamicBlock()
    {
        static constexpr std::array<int, 19> order{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        const int nlen = bits(5) + 257, ndist = bits(5) + 1, ncode = bits(4) + 4;
        if (m_error || nlen > 286 || ndist > 30) return false;
        int lengths[320] = {};
        for (int i = 0; i < ncode; ++i) lengths[order[i]] = bits(3);
        if (m_error) return false;
        Huffman lencode;
        if (!lencode.build(lengths, 19)) return false;
        int index = 0;
        int all[320] = {};
        while (index < nlen + ndist)
        {
            const int sym = decode(lencode);
            if (sym < 0) return false;
            if (sym < 16) all[index++] = sym;
            else
            {
                int len = 0, repeat = 0;
                if (sym == 16)
                {
                    if (index == 0) return false;
                    len = all[index - 1];
                    repeat = 3 + bits(2);
                }
                else if (sym == 17) repeat = 3 + bits(3);
                else repeat = 11 + bits(7);
                if (m_error || index + repeat > nlen + ndist) return false;
                while (repeat--) all[index++] = len;
            }
        }
        if (all[256] == 0) return false;
        Huffman lit, dist;
        if (!lit.build(all, nlen) || !dist.build(all + nlen, ndist)) return false;
        return codes(lit, dist);
    }

    const unsigned char *m_in;
    qint64 m_size;
    qint64 m_maxOut;
    qint64 m_pos{0};
    std::uint32_t m_bitBuffer{0};
    int m_bitCount{0};
    bool m_error{false};
    QByteArray m_out;
};

std::uint32_t le(const QByteArray &d, qint64 pos, int size)
{
    std::uint32_t v = 0;
    for (int i = 0; i < size; ++i) v |= static_cast<std::uint32_t>(static_cast<unsigned char>(d[pos + i])) << (8 * i);
    return v;
}

}  // namespace

bool isZip(const QByteArray &data)
{
    return data.size() >= 4 && data.startsWith("PK\x03\x04");
}

std::optional<QByteArray> zipEntry(const QByteArray &zip, const QString &name, qint64 maxSize)
{
    // end of central directory: the last 22+ bytes, searched backwards (a comment may follow it)
    qint64 eocd = -1;
    for (qint64 p = zip.size() - 22; p >= 0 && p >= zip.size() - 22 - 65535; --p)
        if (le(zip, p, 4) == 0x06054b50u)
        {
            eocd = p;
            break;
        }
    if (eocd < 0) return std::nullopt;
    const int entries = static_cast<int>(le(zip, eocd + 10, 2));
    qint64 pos = le(zip, eocd + 16, 4);
    for (int i = 0; i < entries; ++i)
    {
        if (pos < 0 || pos + 46 > zip.size() || le(zip, pos, 4) != 0x02014b50u) return std::nullopt;
        const int method = static_cast<int>(le(zip, pos + 10, 2));
        const qint64 compSize = le(zip, pos + 20, 4), size = le(zip, pos + 24, 4);
        const int nameLen = static_cast<int>(le(zip, pos + 28, 2)), extraLen = static_cast<int>(le(zip, pos + 30, 2)),
                  commentLen = static_cast<int>(le(zip, pos + 32, 2));
        const qint64 local = le(zip, pos + 42, 4);
        if (pos + 46 + nameLen > zip.size()) return std::nullopt;
        const QString entryName = QString::fromUtf8(zip.constData() + pos + 46, nameLen);
        pos += 46 + nameLen + extraLen + commentLen;
        if (entryName != name) continue;
        if (size > maxSize || local + 30 > zip.size() || le(zip, local, 4) != 0x04034b50u) return std::nullopt;
        const qint64 data = local + 30 + le(zip, local + 26, 2) + le(zip, local + 28, 2);
        if (data + compSize > zip.size()) return std::nullopt;
        if (method == 0) return compSize == size ? std::optional<QByteArray>(zip.mid(data, compSize)) : std::nullopt;
        if (method != 8) return std::nullopt;
        auto out = Inflater(reinterpret_cast<const unsigned char *>(zip.constData()) + data, compSize, maxSize).run();
        if (out && out->size() != size) return std::nullopt;
        return out;
    }
    return std::nullopt;
}

}  // namespace csr
