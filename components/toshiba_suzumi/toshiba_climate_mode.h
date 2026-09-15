#pragma once

#include <cstdint>
#include "esphome/core/log.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace toshiba_suzumi {

constexpr const char* CUSTOM_FAN_LEVEL_2 = "Low-Medium";
constexpr const char* CUSTOM_FAN_LEVEL_4 = "Medium-High";

constexpr const char* CUSTOM_PWR_LEVEL_50 = "50 %";
constexpr const char* CUSTOM_PWR_LEVEL_75 = "75 %";
constexpr const char* CUSTOM_PWR_LEVEL_100 = "100 %";

constexpr const char* SPECIAL_MODE_STANDARD = "Standard";
constexpr const char* SPECIAL_MODE_HI_POWER = "Hi POWER";
constexpr const char* SPECIAL_MODE_ECO = "ECO";
constexpr const char* SPECIAL_MODE_FIREPLACE_1 = "Fireplace 1";
constexpr const char* SPECIAL_MODE_FIREPLACE_2 = "Fireplace 2";
constexpr const char* SPECIAL_MODE_EIGHT_DEG = "8 degrees";
constexpr const char* SPECIAL_MODE_SILENT_1 = "Silent#1";
constexpr const char* SPECIAL_MODE_SILENT_2 = "Silent#2";
constexpr const char* SPECIAL_MODE_SLEEP = "Sleep";
constexpr const char* SPECIAL_MODE_FLOOR = "Floor";
constexpr const char* SPECIAL_MODE_COMFORT = "Comfort";

enum class MODE { HEAT_COOL = 65, COOL = 66, HEAT = 67, DRY = 68, FAN_ONLY = 69 };
enum class FAN {
  FAN_QUIET = 49,
  FAN_LOW = 50,
  FANMODE_2 = 51,
  FAN_MEDIUM = 52,
  FANMODE_4 = 53,
  FAN_HIGH = 54,
  FAN_AUTO = 65
};

// A3 ordinary pushed/read state values. These are NOT the same encoding as the
// genuine adaptor's swing/FIX write commands.
enum class SWING {
  OFF = 0x31,
  VERTICAL = 0x41,
  HORIZONTAL = 0x42,
  BOTH = 0x43,

  // Representative packed FIX values with H=1. The actual command must retain
  // the other axis and is constructed with EncodePackedFixPosition(). Toshiba
  // exposes five user-selectable FIX positions per axis; field value 0 is an
  // observed baseline/unselected state, not a sixth UI position.
  VERTICAL_FIX_POSITION_1 = 0x89,
  VERTICAL_FIX_POSITION_2 = 0x8A,
  VERTICAL_FIX_POSITION_3 = 0x8B,
  VERTICAL_FIX_POSITION_4 = 0x8C,
  VERTICAL_FIX_POSITION_5 = 0x8D,
  HADA = 0x60
};

// Genuine RB-N106S-G A3 commands captured on the tested P2 unit.
static constexpr uint8_t A3_CMD_OFF_TRANSITION = 0x80;
static constexpr uint8_t A3_CMD_BOTH_SWING = 0x80;
static constexpr uint8_t A3_CMD_VERTICAL_SWING = 0xAE;
static constexpr uint8_t A3_CMD_HORIZONTAL_SWING = 0xB6;

enum class STATE { ON = 48, OFF = 49 };
enum class PWR_LEVEL { PCT_50 = 50, PCT_75 = 75, PCT_100 = 100 };

// Register 0xCB is a shared maintenance-cycle state/control register. Defrost
// and indoor self-clean are different operations encoded in the same register.
enum class MAINTENANCE_STATE : uint8_t {
  IDLE = 0x10,
  STRONG_DEFROST = 0x11,
  NORMAL_DEFROST = 0x12,
  SELF_CLEAN = 0x18,
};

// Retained for the legacy base parser; the validated path uses
// MAINTENANCE_STATE so self-clean and defrost remain distinct logical states.
enum class SELF_CLEAN_STATE : uint8_t { RUNNING = 0x18, OFF = 0x10 };

enum SPECIAL_MODE {
  STANDARD = 0,
  HI_POWER = 1,
  ECO = 3,
  FIREPLACE_1 = 32,
  FIREPLACE_2 = 48,
  EIGHT_DEG = 4,
  SILENT_1 = 2,
  SILENT_2 = 10,
  SLEEP = 5,
  FLOOR = 6,
  COMFORT = 7
};

enum class ToshibaCommandType : uint8_t {
  HANDSHAKE = 0,
  DELAY = 1,
  POWER_STATE = 128,
  POWER_SEL = 135,
  TIMER_OFF = 148,
  COMFORT_SLEEP = 148,
  FAN = 160,
  SWING = 163,
  MODE = 176,
  TARGET_TEMP = 179,
  ROOM_TEMP = 187,
  OUTDOOR_TEMP = 190,
  PURE = 0xC7,
  MAINTENANCE = 0xCB,
  DEFROST = 0xCB,
  SELF_CLEAN = 0xCB,
  ENERGY_DAILY = 0xD8,
  ENERGY_WEEKLY = 0xD9,
  ENERGY_MONTHLY = 0xDA,
  ENERGY_YEARLY = 0xDB,
  WIFI_LED_1 = 0xDE,
  WIFI_LED_2 = 0xDF,
  EQUIPMENT_INFO = 0xE0,
  SET_DATE_TIME = 0xEA,
  IDU_STATUS = 0xE4,
  ODU_STATUS = 0xE5,
  SPECIAL_MODE = 247,
};

const MODE ClimateModeToInt(climate::ClimateMode mode);
const climate::ClimateMode IntToClimateMode(MODE mode);

const uint8_t ClimateSwingModeToCommand(climate::ClimateSwingMode mode);
inline SWING ClimateSwingModeToInt(climate::ClimateSwingMode mode) {
  return static_cast<SWING>(ClimateSwingModeToCommand(mode));
}
const climate::ClimateSwingMode IntToClimateSwingMode(SWING mode);

const optional<FAN> ClimateFanModeToInt(climate::ClimateFanMode mode);
const LogString *climate_state_to_string(STATE mode);
const optional<FAN> StringToFanLevel(const char* mode);
const char* IntToCustomFanMode(FAN mode);
const optional<PWR_LEVEL> StringToPwrLevel(const std::string &mode);
const std::string IntToPowerLevel(PWR_LEVEL mode);

const optional<SWING> StringToVerticalAirDirection(const std::string &position);
const char* SwingToVerticalAirDirection(SWING mode);
bool IsFixedVerticalAirDirection(SWING mode);
bool DecodePackedFixPosition(uint8_t raw, uint8_t &horizontal_index, uint8_t &vertical_index);
uint8_t EncodePackedFixPosition(uint8_t horizontal_index, uint8_t vertical_index);
const char *FixedPositionName(uint8_t position_index);
const optional<uint8_t> FixedPositionIndexFromName(const std::string &value);

const optional<SPECIAL_MODE> PresetToSpecialMode(const char* preset);
const char* SpecialModeToPreset(SPECIAL_MODE mode);
const optional<climate::ClimatePreset> StringToClimatePreset(const char *preset);
const char* ClimatePresetToString(climate::ClimatePreset preset);
const optional<SPECIAL_MODE> ClimatePresetToSpecialMode(climate::ClimatePreset preset);
const optional<climate::ClimatePreset> SpecialModeToClimatePreset(SPECIAL_MODE mode);

}  // namespace toshiba_suzumi
}  // namespace esphome
