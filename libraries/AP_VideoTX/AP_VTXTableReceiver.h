/*
 * JTV1 bounded chunk reassembly. No heap, filesystem or MAVLink dependency.
 * A transport must authenticate/authorize the sender, enforce disarmed state,
 * and invoke reset() on session change or timeout.
 */
#pragma once
#include "AP_VTXTableJTV1.h"

namespace JTV1 {
class Receiver {
public:
    static constexpr size_t MAX_CHUNK = 64;
    void reset()
    {
        memset(bytes_, 0, sizeof(bytes_));
        memset(received_, 0, sizeof(received_));
        count_ = 0;
        ready_ = false;
    }

    Receiver() { reset(); }

    // Supports retransmission and out-of-order chunks. Rejects conflicting
    // retransmissions; a new upload must explicitly reset the session.
    bool write(size_t offset, const uint8_t *data, size_t length)
    {
        if (ready_ || data == nullptr || length == 0 || length > MAX_CHUNK ||
            offset >= SIZE || length > SIZE - offset) {
            return false;
        }
        for (size_t i = 0; i < length; i++) {
            const size_t pos = offset + i;
            if (has(pos) && bytes_[pos] != data[i]) {
                return false;
            }
        }
        for (size_t i = 0; i < length; i++) {
            const size_t pos = offset + i;
            if (!has(pos)) {
                received_[pos / 8] |= uint8_t(1U << (pos % 8));
                count_++;
            }
            bytes_[pos] = data[i];
        }
        return true;
    }

    bool complete() const { return count_ == SIZE; }
    bool validate(Table &out)
    {
        if (!complete()) {
            return false;
        }
        if (!decode(bytes_, SIZE, out)) {
            return false;
        }
        ready_ = true;
        return true;
    }

private:
    bool has(size_t index) const
    {
        return (received_[index / 8] & uint8_t(1U << (index % 8))) != 0;
    }
    uint8_t bytes_[SIZE];
    uint8_t received_[(SIZE + 7) / 8];
    size_t count_;
    bool ready_;
};
} // namespace JTV1
