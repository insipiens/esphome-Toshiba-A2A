#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace toshiba_a2a {

static constexpr size_t TOSHIBA_FRAME_LENGTH_FIELD_SIZE = 7;
static constexpr size_t TOSHIBA_FRAME_OVERHEAD = 8;
static constexpr size_t TOSHIBA_MAX_FRAME_LENGTH = 4096;

inline size_t toshiba_declared_frame_length(const uint8_t *data, size_t size) {
  if (data == nullptr || size < TOSHIBA_FRAME_LENGTH_FIELD_SIZE) return 0;

  const size_t protocol_length =
      (static_cast<size_t>(data[5]) << 8) | data[6];
  return protocol_length + TOSHIBA_FRAME_OVERHEAD;
}

}  // namespace toshiba_a2a
}  // namespace esphome
