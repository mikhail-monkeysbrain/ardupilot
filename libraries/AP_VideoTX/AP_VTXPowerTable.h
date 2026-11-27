#pragma once

#include <stdint.h>

namespace JTPower {

static constexpr uint8_t MAX_LEVELS = 10;

enum class Protocol : uint8_t {
    SmartAudio1 = 1,
    SmartAudio2 = 2,
    SmartAudio21 = 3,
    Tramp = 4
};

struct Level {
    uint16_t code;
    uint32_t label;
};

struct Table {
    Protocol protocol;
    uint8_t count;
    Level levels[MAX_LEVELS];
};

inline bool valid_code(Protocol protocol, uint16_t code)
{
    switch (protocol) {
    case Protocol::SmartAudio1:
    case Protocol::SmartAudio2:
        return code <= 255;
    case Protocol::SmartAudio21:
        return code <= 127;
    case Protocol::Tramp:
        return true;
    }
    return false;
}

inline bool validate(const Table &table)
{
    if (table.count == 0 || table.count > MAX_LEVELS) {
        return false;
    }
    for (uint8_t i = 0; i < table.count; i++) {
        if (!valid_code(table.protocol, table.levels[i].code) ||
            table.levels[i].label > 0xFFFFFFU) {
            return false;
        }
        for (uint8_t j = 0; j < i; j++) {
            if (table.levels[i].code == table.levels[j].code) {
                return false;
            }
        }
    }
    return true;
}

inline int8_t find_code(const Table &table, uint16_t code)
{
    if (!validate(table)) {
        return -1;
    }
    for (uint8_t i = 0; i < table.count; i++) {
        if (table.levels[i].code == code) {
            return int8_t(i);
        }
    }
    return -1;
}

inline bool encode(const Table &table, uint8_t index, uint16_t &wire)
{
    if (!validate(table) || index >= table.count) {
        return false;
    }
    wire = table.levels[index].code;
    if (table.protocol == Protocol::SmartAudio21) {
        wire |= 0x80U;
    }
    return true;
}

inline bool decode_label(uint32_t value, char (&out)[5])
{
    static constexpr char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";
    out[0] = 0;
    if (value == 0 || value > 0xFFFFFFU) {
        return false;
    }
    bool ended = false;
    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t symbol = (value >> (6U * i)) & 63U;
        if (symbol == 0) {
            ended = true;
        } else if (ended) {
            out[0] = 0;
            return false;
        }
        out[i] = symbol == 0 ? char(0) : alphabet[symbol - 1U];
    }
    out[4] = 0;
    return true;
}

inline uint32_t checksum(const Table &table)
{
    if (!validate(table)) {
        return 0;
    }
    uint32_t h = 2166136261U;
    const auto add = [&h](uint8_t byte) {
        h = (h ^ byte) * 16777619U;
    };
    add(uint8_t(table.protocol));
    add(table.count);
    for (uint8_t i = 0; i < MAX_LEVELS; i++) {
        const uint16_t code = i < table.count ? table.levels[i].code : 0;
        const uint32_t label = i < table.count ? table.levels[i].label : 0;
        add(uint8_t(code));
        add(uint8_t(code >> 8));
        add(uint8_t(label));
        add(uint8_t(label >> 8));
        add(uint8_t(label >> 16));
    }
    return (h & 0xFFFFFFU) | 1U;
}

} // namespace JTPower
