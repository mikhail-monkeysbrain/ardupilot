/*
 * JTZero VTX table JTV1 binary decoder.
 * Pure C++11, no dynamic allocations or hardware access.
 * This parses a candidate table only; it does NOT activate or persist it.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace JTV1 {
static constexpr size_t HEADER_SIZE = 12;
static constexpr uint8_t MAX_BANDS = 11;
static constexpr uint8_t MAX_CHANNELS = 8;
static constexpr uint8_t MAX_POWERS = 10;
static constexpr size_t SIZE = HEADER_SIZE + MAX_BANDS * MAX_CHANNELS * 2 + MAX_POWERS * 4 + 4;

struct Power {
    uint16_t protocol_code;
    uint16_t mw; // 0 = unknown; NEVER infer physical power from protocol code
};

struct Table {
    uint8_t bands;
    uint8_t channels;
    uint8_t powers;
    uint16_t frequency[MAX_BANDS][MAX_CHANNELS];
    Power power[MAX_POWERS];
};

inline uint16_t read_u16(const uint8_t *p)
{
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

inline uint32_t read_u32(const uint8_t *p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

inline uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0U);
        }
    }
    return ~crc;
}

inline bool decode(const uint8_t *data, size_t length, Table &out)
{
    if (data == nullptr || length != SIZE ||
        memcmp(data, "JTV1", 4) != 0 ||
        read_u16(data + 4) != 1 ||
        read_u16(data + 6) != SIZE ||
        data[9] != MAX_CHANNELS || data[11] != 0 ||
        data[8] == 0 || data[8] > MAX_BANDS ||
        data[10] > MAX_POWERS ||
        read_u32(data + SIZE - 4) != crc32(data, SIZE - 4)) {
        return false;
    }

    Table candidate{};
    candidate.bands = data[8];
    candidate.channels = data[9];
    candidate.powers = data[10];
    size_t pos = HEADER_SIZE;
    for (uint8_t b = 0; b < MAX_BANDS; b++) {
        for (uint8_t c = 0; c < MAX_CHANNELS; c++) {
            const uint16_t freq = read_u16(data + pos);
            pos += 2;
            if ((freq != 0 && (freq < 100)) ||
                (b >= candidate.bands && freq != 0)) {
                return false;
            }
            candidate.frequency[b][c] = freq;
        }
    }
    for (uint8_t i = 0; i < MAX_POWERS; i++) {
        const uint16_t code = read_u16(data + pos);
        const uint16_t mw = read_u16(data + pos + 2);
        pos += 4;
        if (i >= candidate.powers && (code != 0 || mw != 0)) {
            return false;
        }
        candidate.power[i] = {code, mw};
    }
    // Commit output only after every field and CRC has passed validation.
    out = candidate;
    return true;
}
} // namespace JTV1
