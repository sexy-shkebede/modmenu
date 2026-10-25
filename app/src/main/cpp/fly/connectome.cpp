#include "connectome.hpp"

#include <stdio.h>
#include <string.h>
#include <zlib.h>

namespace
{
    struct SHeader { char magic[4]; uint32_t uVersion, N, M, uFlags, uSize; };
    static_assert(sizeof(SHeader) == 24, "header layout");
}

const std::vector<uint32_t>* fly::SConnectome::Group(const char* pszName) const
{
    for (const SGroup& g : groups)
        if (g.sName == pszName) return &g.idx;
    return nullptr;
}

bool fly::SConnectome::Load(const char* pszPath, std::string& sError, volatile float* pProgress)
{
    FILE* f = fopen(pszPath, "rb");
    if (!f) { sError = "can't open file"; return false; }
    fseek(f, 0, SEEK_END);
    const long nFile = ftell(f);
    fseek(f, 0, SEEK_SET);

    SHeader h {};
    if (fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "FLY1", 4) != 0 || h.uVersion != 1 || h.N == 0 || h.N >= (1u << 18))
    {
        fclose(f);
        sError = "not a fly_brain.bin (version 1)";
        return false;
    }
    m_Data.assign((h.uSize + 3) / 4, 0u);
    unsigned char* pOut = reinterpret_cast<unsigned char*>(m_Data.data());

    bool bOk = true;
    if (!(h.uFlags & 1))
        bOk = fread(pOut, 1, h.uSize, f) == h.uSize;
    else
    {
        // Распаковка zlib кусками: в памяти нет второй копии файла
        z_stream z {};
        bOk = inflateInit(&z) == Z_OK;
        z.next_out  = pOut;
        z.avail_out = h.uSize;
        std::vector<unsigned char> chunk(1 << 18);
        int r = Z_OK;
        while (bOk && r != Z_STREAM_END)
        {
            z.avail_in = (uInt)fread(chunk.data(), 1, chunk.size(), f);
            z.next_in  = chunk.data();
            if (z.avail_in == 0) { bOk = false; break; }
            while (z.avail_in > 0 && r != Z_STREAM_END)
            {
                r = inflate(&z, Z_NO_FLUSH);
                if (r != Z_OK && r != Z_STREAM_END) { bOk = false; break; }
            }
            if (pProgress && nFile > 0) *pProgress = 0.95f * (float)ftell(f) / (float)nFile;
        }
        bOk = bOk && z.total_out == h.uSize;
        inflateEnd(&z);
    }
    fclose(f);
    if (!bOk) { sError = "file is damaged or incomplete"; m_Data.clear(); return false; }

    // Разметка: cptr, syn, pos, super, группы
    const size_t nCptr = (size_t)h.N + 1, nSyn = h.M;
    size_t off = 0;
    const auto Need = [&](size_t n) { return off + n <= h.uSize; };
    if (!Need(nCptr * 4 + nSyn * 4 + (size_t)h.N * 6 + h.N + 4)) { sError = "file is truncated"; return false; }
    pCptr  = reinterpret_cast<const uint32_t*>(pOut + off); off += nCptr * 4;
    pSyn   = reinterpret_cast<const uint32_t*>(pOut + off); off += nSyn * 4;
    pPos   = reinterpret_cast<const int16_t*>(pOut + off);  off += (size_t)h.N * 6;
    pSuper = pOut + off;                                     off += h.N;
    if (pCptr[h.N] != h.M) { sError = "connection table doesn't match its header"; return false; }

    uint32_t nGroups = 0;
    memcpy(&nGroups, pOut + off, 4); off += 4;
    groups.clear();
    for (uint32_t k = 0; k < nGroups; k++)
    {
        if (!Need(1)) { sError = "group table is truncated"; return false; }
        const uint8_t nName = pOut[off++];
        if (!Need(nName + 4u)) { sError = "group table is truncated"; return false; }
        SGroup g;
        g.sName.assign(reinterpret_cast<const char*>(pOut + off), nName); off += nName;
        uint32_t n = 0;
        memcpy(&n, pOut + off, 4); off += 4;
        if (!Need((size_t)n * 4)) { sError = "group table is truncated"; return false; }
        g.idx.resize(n);
        memcpy(g.idx.data(), pOut + off, (size_t)n * 4); off += (size_t)n * 4;
        for (uint32_t i : g.idx) if (i >= h.N) { sError = "group refers to a missing neuron"; return false; }
        groups.push_back(std::move(g));
    }
    N = h.N;
    M = h.M;
    if (pProgress) *pProgress = 1.f;
    return true;
}
