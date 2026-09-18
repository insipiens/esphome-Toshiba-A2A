#pragma once

#include <cstdint>

namespace esphome {
namespace toshiba_a2a {

enum class ToshibaQueueOperation : uint8_t {
  REGISTER = 0,
  HANDSHAKE,
  DELAY,
};

enum class ToshibaRegister : uint8_t {
  POWER_STATE = 0x80,
  POWER_SELECT = 0x87,
  TIMER_OFF = 0x94,
  FAN = 0xA0,
  LOUVRE = 0xA3,
  MODE = 0xB0,
  TARGET_TEMP = 0xB3,
  ROOM_TEMP = 0xBB,
  OUTDOOR_TEMP = 0xBE,
  PURE = 0xC7,
  MAINTENANCE = 0xCB,
  ENERGY_DAILY = 0xD8,
  ENERGY_WEEKLY = 0xD9,
  ENERGY_MONTHLY = 0xDA,
  ENERGY_YEARLY = 0xDB,
  WIFI_LED_1 = 0xDE,
  WIFI_LED_2 = 0xDF,
  EQUIPMENT_INFO = 0xE0,
  IDU_STATUS = 0xE4,
  ODU_STATUS = 0xE5,
  SET_DATE_TIME = 0xEA,
  SPECIAL_MODE = 0xF7,
};

namespace reg_80 {
enum class State : uint8_t {
  ON = 0x30,
  OFF = 0x31,
};
}  // namespace reg_80

namespace reg_87 {
enum class PowerLevel : uint8_t {
  PCT_50 = 50,
  PCT_75 = 75,
  PCT_100 = 100,
};
}  // namespace reg_87

namespace reg_a0 {
enum class Fan : uint8_t {
  FAN_QUIET = 0x31,
  FAN_LOW = 0x32,
  FANMODE_2 = 0x33,
  FAN_MEDIUM = 0x34,
  FANMODE_4 = 0x35,
  FAN_HIGH = 0x36,
  FAN_AUTO = 0x41,
};
}  // namespace reg_a0

namespace reg_b0 {
enum class Mode : uint8_t {
  HEAT_COOL = 0x41,
  COOL = 0x42,
  HEAT = 0x43,
  DRY = 0x44,
  FAN_ONLY = 0x45,
};
}  // namespace reg_b0

namespace reg_c7 {
enum class PureState : uint8_t {
  OFF = 0x10,
  ON = 0x18,
};
}  // namespace reg_c7

namespace reg_cb {
enum class State : uint8_t {
  IDLE = 0x10,
  STRONG_DEFROST = 0x11,
  NORMAL_DEFROST = 0x12,
  SELF_CLEAN = 0x18,
};

enum class Command : uint8_t {
  STRONG_DEFROST = 0x01,
  NORMAL_DEFROST = 0x02,
};

enum class SelfCleanState : uint8_t {
  RUNNING = 0x18,
  OFF = 0x10,
};
}  // namespace reg_cb

namespace reg_f7 {
enum class SpecialMode : uint8_t {
  STANDARD = 0x00,
  HI_POWER = 0x01,
  SILENT_1 = 0x02,
  ECO = 0x03,
  EIGHT_DEG = 0x04,
  SLEEP = 0x05,
  FLOOR = 0x06,
  COMFORT = 0x07,
  SILENT_2 = 0x0A,
  FIREPLACE_1 = 0x20,
  FIREPLACE_2 = 0x30,
};
}  // namespace reg_f7

static_assert(static_cast<uint8_t>(ToshibaRegister::POWER_STATE) == 0x80);
static_assert(static_cast<uint8_t>(ToshibaRegister::FAN) == 0xA0);
static_assert(static_cast<uint8_t>(ToshibaRegister::LOUVRE) == 0xA3);
static_assert(static_cast<uint8_t>(ToshibaRegister::MODE) == 0xB0);
static_assert(static_cast<uint8_t>(ToshibaRegister::SPECIAL_MODE) == 0xF7);

}  // namespace toshiba_a2a
}  // namespace esphome
