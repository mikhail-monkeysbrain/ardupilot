#pragma once
#include <stdint.h>

namespace JTVTX {
class Retry {
public:
    static constexpr uint8_t MAX_ATTEMPTS = 5;
    bool available(uint16_t value) const {
        return !_valid || _value != value || _attempts < MAX_ATTEMPTS;
    }
    bool allow(uint16_t value) {
        if (!_valid || _value != value) {
            _valid = true;
            _value = value;
            _attempts = 0;
        }
        if (_attempts >= MAX_ATTEMPTS) {
            return false;
        }
        ++_attempts;
        return true;
    }
    void reset() {
        _valid = false;
        _attempts = 0;
    }
    uint8_t attempts() const { return _attempts; }
private:
    uint16_t _value = 0;
    uint8_t _attempts = 0;
    bool _valid = false;
};
}
