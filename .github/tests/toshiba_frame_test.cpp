#include <cassert>
#include <cstdint>

#include "components/toshiba_a2a/toshiba_frame.h"

using esphome::toshiba_a2a::toshiba_declared_frame_length;

int main() {
  const uint8_t short_frame[] = {0x02, 0x00, 0x03, 0x10, 0x00, 0x00, 0x07};
  const uint8_t cc_frame[] = {0x02, 0x00, 0x03, 0x90, 0x00, 0x01, 0xEE};
  const uint8_t ce_frame[] = {0x02, 0x00, 0x03, 0x90, 0x00, 0x03, 0x72};
  const uint8_t cf_frame[] = {0x02, 0x00, 0x03, 0x90, 0x00, 0x01, 0x5E};
  const uint8_t incomplete[] = {0x02, 0x00, 0x03, 0x90, 0x00, 0x03};

  assert(toshiba_declared_frame_length(short_frame, sizeof(short_frame)) == 15);
  assert(toshiba_declared_frame_length(cc_frame, sizeof(cc_frame)) == 502);
  assert(toshiba_declared_frame_length(ce_frame, sizeof(ce_frame)) == 890);
  assert(toshiba_declared_frame_length(cf_frame, sizeof(cf_frame)) == 358);
  assert(toshiba_declared_frame_length(incomplete, sizeof(incomplete)) == 0);

  return 0;
}
