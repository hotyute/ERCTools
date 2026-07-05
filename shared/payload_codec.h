#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <compressapi.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace erc::payload
{
constexpr uint32_t kRaw = 0;
constexpr uint32_t kXpressHuff = 1;
constexpr uint32_t kLzBlock = 2;
constexpr size_t kCompressionThreshold = 512;

struct Encoded
{
    uint32_t method = kRaw;
    uint32_t originalSize = 0;
    std::vector<unsigned char> bytes;
};

struct CompressionApi
{
    using CreateCompressorFn = BOOL(WINAPI*)(DWORD, PCOMPRESS_ALLOCATION_ROUTINES, PCOMPRESSOR_HANDLE);
    using CompressFn = BOOL(WINAPI*)(COMPRESSOR_HANDLE, LPCVOID, SIZE_T, PVOID, SIZE_T, PSIZE_T);
    using CloseCompressorFn = BOOL(WINAPI*)(COMPRESSOR_HANDLE);
    using CreateDecompressorFn = BOOL(WINAPI*)(DWORD, PCOMPRESS_ALLOCATION_ROUTINES, PDECOMPRESSOR_HANDLE);
    using DecompressFn = BOOL(WINAPI*)(DECOMPRESSOR_HANDLE, LPCVOID, SIZE_T, PVOID, SIZE_T, PSIZE_T);
    using CloseDecompressorFn = BOOL(WINAPI*)(DECOMPRESSOR_HANDLE);

    HMODULE module = nullptr;
    CreateCompressorFn createCompressor = nullptr;
    CompressFn compress = nullptr;
    CloseCompressorFn closeCompressor = nullptr;
    CreateDecompressorFn createDecompressor = nullptr;
    DecompressFn decompress = nullptr;
    CloseDecompressorFn closeDecompressor = nullptr;

    CompressionApi()
    {
        module = LoadLibraryW(L"cabinet.dll");
        if (!module)
            return;
        createCompressor = reinterpret_cast<CreateCompressorFn>(GetProcAddress(module, "CreateCompressor"));
        compress = reinterpret_cast<CompressFn>(GetProcAddress(module, "Compress"));
        closeCompressor = reinterpret_cast<CloseCompressorFn>(GetProcAddress(module, "CloseCompressor"));
        createDecompressor = reinterpret_cast<CreateDecompressorFn>(GetProcAddress(module, "CreateDecompressor"));
        decompress = reinterpret_cast<DecompressFn>(GetProcAddress(module, "Decompress"));
        closeDecompressor = reinterpret_cast<CloseDecompressorFn>(GetProcAddress(module, "CloseDecompressor"));
    }

    bool CanCompress() const { return createCompressor && compress && closeCompressor; }
    bool CanDecompress() const { return createDecompressor && decompress && closeDecompressor; }
};

inline CompressionApi& Api()
{
    static CompressionApi api;
    return api;
}

inline void AppendExtendedLength(std::vector<unsigned char>& output, size_t length)
{
    while (length >= 255) {
        output.push_back(255);
        length -= 255;
    }
    output.push_back(static_cast<unsigned char>(length));
}

inline std::vector<unsigned char> EncodeLzBlock(const std::string& input)
{
    constexpr size_t kHashSize = 1u << 16;
    std::array<int, kHashSize> table{};
    table.fill(-1);
    std::vector<unsigned char> output;
    output.reserve(input.size() / 2);
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
    const size_t size = input.size();
    size_t anchor = 0;
    size_t cursor = 0;

    const auto hashAt = [&](size_t position) {
        uint32_t value = 0;
        std::memcpy(&value, data + position, sizeof(value));
        return static_cast<size_t>((value * 2654435761u) >> 16);
    };

    while (cursor + 4 <= size) {
        const size_t hash = hashAt(cursor);
        const int candidate = table[hash];
        table[hash] = static_cast<int>(cursor);
        if (candidate < 0 || cursor - static_cast<size_t>(candidate) > 65535 ||
            std::memcmp(data + candidate, data + cursor, 4) != 0)
        {
            ++cursor;
            continue;
        }

        const size_t literalLength = cursor - anchor;
        size_t matchLength = 4;
        while (cursor + matchLength < size && data[candidate + matchLength] == data[cursor + matchLength])
            ++matchLength;

        const size_t tokenPosition = output.size();
        output.push_back(0);
        output[tokenPosition] = static_cast<unsigned char>(
            (std::min<size_t>(literalLength, 15) << 4) |
            std::min<size_t>(matchLength - 4, 15));
        if (literalLength >= 15)
            AppendExtendedLength(output, literalLength - 15);
        output.insert(output.end(), data + anchor, data + cursor);

        const size_t offset = cursor - static_cast<size_t>(candidate);
        output.push_back(static_cast<unsigned char>(offset & 0xff));
        output.push_back(static_cast<unsigned char>((offset >> 8) & 0xff));
        if (matchLength - 4 >= 15)
            AppendExtendedLength(output, matchLength - 4 - 15);

        cursor += matchLength;
        anchor = cursor;
        if (cursor >= 2 && cursor + 4 <= size) {
            table[hashAt(cursor - 2)] = static_cast<int>(cursor - 2);
            table[hashAt(cursor - 1)] = static_cast<int>(cursor - 1);
        }
    }

    const size_t literalLength = size - anchor;
    const size_t tokenPosition = output.size();
    output.push_back(static_cast<unsigned char>(std::min<size_t>(literalLength, 15) << 4));
    if (literalLength >= 15)
        AppendExtendedLength(output, literalLength - 15);
    output.insert(output.end(), data + anchor, data + size);
    return output;
}

inline bool ReadExtendedLength(
    const std::vector<unsigned char>& input,
    size_t& cursor,
    size_t& length)
{
    for (;;) {
        if (cursor >= input.size())
            return false;
        const unsigned char value = input[cursor++];
        length += value;
        if (value != 255)
            return true;
    }
}

inline bool DecodeLzBlock(
    const std::vector<unsigned char>& input,
    uint32_t originalSize,
    std::string& output)
{
    std::vector<unsigned char> decoded;
    decoded.reserve(originalSize);
    size_t cursor = 0;
    while (cursor < input.size()) {
        const unsigned char token = input[cursor++];
        size_t literalLength = token >> 4;
        if (literalLength == 15 && !ReadExtendedLength(input, cursor, literalLength))
            return false;
        if (literalLength > input.size() - cursor || decoded.size() + literalLength > originalSize)
            return false;
        decoded.insert(decoded.end(), input.begin() + cursor, input.begin() + cursor + literalLength);
        cursor += literalLength;
        if (cursor == input.size())
            break;
        if (cursor + 2 > input.size())
            return false;
        const size_t offset = static_cast<size_t>(input[cursor]) |
            (static_cast<size_t>(input[cursor + 1]) << 8);
        cursor += 2;
        if (offset == 0 || offset > decoded.size())
            return false;
        size_t matchLength = (token & 0x0f) + 4;
        if ((token & 0x0f) == 15 && !ReadExtendedLength(input, cursor, matchLength))
            return false;
        if (decoded.size() + matchLength > originalSize)
            return false;
        const size_t matchStart = decoded.size() - offset;
        for (size_t i = 0; i < matchLength; ++i)
            decoded.push_back(decoded[matchStart + i]);
    }
    if (decoded.size() != originalSize)
        return false;
    output.assign(decoded.begin(), decoded.end());
    return true;
}

inline Encoded Encode(const std::string& input)
{
    Encoded result;
    result.originalSize = static_cast<uint32_t>(std::min<size_t>(input.size(), std::numeric_limits<uint32_t>::max()));
    result.bytes.assign(input.begin(), input.end());
    if (input.size() < kCompressionThreshold || input.size() > std::numeric_limits<uint32_t>::max())
        return result;

    std::vector<unsigned char> portable = EncodeLzBlock(input);
    if (!portable.empty() && portable.size() < result.bytes.size()) {
        result.method = kLzBlock;
        result.bytes = std::move(portable);
    }

    CompressionApi& api = Api();
    if (!api.CanCompress())
        return result;
    COMPRESSOR_HANDLE compressor = nullptr;
    if (!api.createCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &compressor))
        return result;

    SIZE_T required = 0;
    api.compress(compressor, input.data(), input.size(), nullptr, 0, &required);
    if (required == 0 || required >= result.bytes.size()) {
        api.closeCompressor(compressor);
        return result;
    }

    std::vector<unsigned char> compressed(required);
    SIZE_T written = 0;
    const BOOL ok = api.compress(
        compressor,
        input.data(),
        input.size(),
        compressed.data(),
        compressed.size(),
        &written);
    api.closeCompressor(compressor);
    if (!ok || written == 0 || written >= result.bytes.size())
        return result;

    compressed.resize(written);
    result.method = kXpressHuff;
    result.bytes = std::move(compressed);
    return result;
}

inline bool Decode(
    uint32_t method,
    uint32_t originalSize,
    const std::vector<unsigned char>& encoded,
    std::string& output)
{
    output.clear();
    if (method == kRaw) {
        output.assign(encoded.begin(), encoded.end());
        return originalSize == 0 || output.size() == originalSize;
    }
    if (method == kLzBlock)
        return originalSize != 0 && DecodeLzBlock(encoded, originalSize, output);
    if (method != kXpressHuff || originalSize == 0)
        return false;

    CompressionApi& api = Api();
    if (!api.CanDecompress())
        return false;
    DECOMPRESSOR_HANDLE decompressor = nullptr;
    if (!api.createDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &decompressor))
        return false;

    std::vector<unsigned char> decoded(originalSize);
    SIZE_T written = 0;
    const BOOL ok = api.decompress(
        decompressor,
        encoded.data(),
        encoded.size(),
        decoded.data(),
        decoded.size(),
        &written);
    api.closeDecompressor(decompressor);
    if (!ok || written != originalSize)
        return false;

    output.assign(decoded.begin(), decoded.end());
    return true;
}
}
