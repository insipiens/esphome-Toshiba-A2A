#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace toshiba_uart_sniffer {

struct StreamState {
  std::vector<uint8_t> buffer;
  uint32_t last_partial_ms{0};
};

inline int frame_register(const std::vector<uint8_t> &frame) {
  if (frame.size() < 4) return -1;
  const uint8_t type = frame[3];
  if ((type == 0x10 || type == 0x11) && frame.size() > 12) return frame[12];
  if (type == 0x90 && frame.size() > 14) return frame[14];
  return -1;
}

inline void log_complete_frame(const char *direction,
                               const std::vector<uint8_t> &frame,
                               uint32_t now) {
  const int reg = frame_register(frame);
  if (reg >= 0) {
    ESP_LOGI("toshiba_sniffer", "%s FRAME t=%ums len=%u reg=%02X",
             direction, static_cast<unsigned>(now),
             static_cast<unsigned>(frame.size()), reg);
  } else {
    ESP_LOGI("toshiba_sniffer", "%s FRAME t=%ums len=%u",
             direction, static_cast<unsigned>(now),
             static_cast<unsigned>(frame.size()));
  }

  constexpr size_t chunk_size = 100;
  const size_t chunks = (frame.size() + chunk_size - 1) / chunk_size;
  for (size_t i = 0; i < chunks; ++i) {
    const size_t first = i * chunk_size;
    const size_t last = std::min(first + chunk_size, frame.size());
    std::vector<uint8_t> chunk(frame.begin() + first, frame.begin() + last);
    ESP_LOGI("toshiba_sniffer", "%s [%u/%u] %s",
             direction, static_cast<unsigned>(i + 1),
             static_cast<unsigned>(chunks),
             format_hex_pretty(chunk).c_str());
  }
}

inline void process(StreamState &state, const std::vector<uint8_t> &bytes,
                    const char *direction) {
  const uint32_t now = millis();

  if (!state.buffer.empty() && state.last_partial_ms != 0 &&
      static_cast<uint32_t>(now - state.last_partial_ms) > 1000) {
    ESP_LOGW("toshiba_sniffer", "%s stale partial t=%ums len=%u %s",
             direction, static_cast<unsigned>(now),
             static_cast<unsigned>(state.buffer.size()),
             format_hex_pretty(state.buffer).c_str());
    state.buffer.clear();
  }

  state.buffer.insert(state.buffer.end(), bytes.begin(), bytes.end());
  state.last_partial_ms = now;

  while (true) {
    if (state.buffer.size() < 3) return;

    size_t start = 0;
    while (start + 2 < state.buffer.size() &&
           !(state.buffer[start] == 0x02 &&
             state.buffer[start + 1] == 0x00 &&
             state.buffer[start + 2] == 0x03)) {
      ++start;
    }

    if (start + 2 >= state.buffer.size()) {
      if (state.buffer.size() > 2) {
        const size_t drop = state.buffer.size() - 2;
        ESP_LOGW("toshiba_sniffer", "%s resync dropped %u byte(s)",
                 direction, static_cast<unsigned>(drop));
        state.buffer.erase(state.buffer.begin(), state.buffer.begin() + drop);
      }
      return;
    }

    if (start > 0) {
      ESP_LOGW("toshiba_sniffer", "%s resync dropped %u byte(s)",
               direction, static_cast<unsigned>(start));
      state.buffer.erase(state.buffer.begin(), state.buffer.begin() + start);
    }

    if (state.buffer.size() < 7) return;

    const size_t protocol_length =
        (static_cast<size_t>(state.buffer[5]) << 8) | state.buffer[6];
    const size_t expected_length = protocol_length + 8;

    if (expected_length > 4096) {
      ESP_LOGW("toshiba_sniffer",
               "%s invalid frame length %u; dropping prefix byte",
               direction, static_cast<unsigned>(expected_length));
      state.buffer.erase(state.buffer.begin());
      continue;
    }

    if (state.buffer.size() < expected_length) return;

    std::vector<uint8_t> frame(state.buffer.begin(),
                               state.buffer.begin() + expected_length);
    state.buffer.erase(state.buffer.begin(),
                       state.buffer.begin() + expected_length);
    log_complete_frame(direction, frame, now);

    if (state.buffer.empty()) {
      state.last_partial_ms = 0;
      return;
    }
  }
}

}  // namespace toshiba_uart_sniffer
