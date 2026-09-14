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
// Special modes described at:
// https://partner.toshiba-klima.at/data/01_RAS/02_Multi/01_R32/01_multi_indoor/02_SHORAI_EDGE_J2KVSG/02_Manuals/OM_RAS_18_B22_B24_J2KVSG_J2AVSG_E_ML.pdf
constexpr const char* SPECIAL_MODE_FIREPLACE_1 = "Fireplace 1";
constexpr const char* SPECIAL_MODE_FIREPLACE_2 = "Fireplace 2";
constexpr const char* SPECIAL_MODE_EIGHT_DEG = "8 degrees";
constexpr const char* SPECIAL_MODE_SILENT_1 = "Silent#1";
constexpr const char* SPECIAL_MODE_SILENT_2 = "Silent#2";
constexpr const char* SPECIAL_MODE_SLEEP = "Sleep";
constexpr const char* SPECIAL_MODE_FLOOR = "Floor";
constexpr const char* SPECIAL_MODE_COMFORT = "Comfort";

// codes as reverse engineered from Toshiba AC communication with original Wifi module.
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

  // Genuine RB-N106S-G P2 vertical FIX writes. The packed value can also be
  // observed as louvre position state, so it remains useful in this enum.
  VERTICAL_FIX_POSITION_1 = 0x88,
  VERTICAL_FIX_POSITION_2 = 0x89,
  VERTICAL_FIX_POSITION_3 = 0x8A,
  VERTICAL_FIX_POSITION_4 = 0x8B,
  VERTICAL_FIX_POSITION_5 = 0x8C,
  VERTICAL_FIX_POSITION_6 = 0x8D,
  HADA = 0x60
};

// Genuine RB-N106S-G A3 commands captured on the tested P2 unit.
// BOTH and OFF_TRANSITION both use 0x80 in captures; the resulting authoritative
// IDU pushed state must therefore be used to distinguish the resulting state.
static constexpr uint8_t A3_CMD_OFF_TRANSITION = 0x80;
static constexpr uint8_t A3_CMD_BOTH_SWING = 0x80;
static constexpr uint8_t A3_CMD_VERTICAL_SWING = 0xAE;
static constexpr uint8_t A3_CMD_HORIZONTAL_SWING = 0xB6;

enum class STATE { ON = 48, OFF = 49 };
enum class PWR_LEVEL { PCT_50 = 50, PCT_75 = 75, PCT_100 = 100 };
// Values documented by maxmacstn/ToshibaCarrierController.
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
  HANDSHAKE = 0,  // dummy command to handle all handshake requests
  DELAY = 1, // dummy command to issue a delay in communication
  POWER_STATE = 128,
  POWER_SEL = 135,
  TIMER_OFF = 148,      // 0x94; observed 0x41 when Timer Off is activated, 0x42 inactive
  COMFORT_SLEEP = 148,  // legacy alias retained temporarily; 0x94 is NOT Comfort Sleep
  FAN = 160,
  SWING = 163,
  MODE = 176,
  TARGET_TEMP = 179,
  ROOM_TEMP = 187,
  OUTDOOR_TEMP = 190,
  PURE = 0xC7,          // observed 0x18 active, 0x10 inactive
  DEFROST = 0xCB,       // P2: commands 00 stop, 01 strong, 02 normal; pushed 10/11 states confirmed
  SELF_CLEAN = 0xCB,    // legacy alias retained for non-P2 behaviour pending separate re-validation
  ENERGY_DAILY = 0xD8,
  ENERGY_WEEKLY = 0xD9,
  ENERGY_MONTHLY = 0xDA,
  ENERGY_YEARLY = 0xDB,
  WIFI_LED_1 = 0xDE,
  WIFI_LED_2 = 0xDF,
  EQUIPMENT_INFO = 0xE0,  // pushed class-0x11 equipment identification
  SET_DATE_TIME = 0xEA,
  IDU_STATUS = 0xE4,   // 228 - Indoor unit status
  ODU_STATUS = 0xE5,   // 229 - Outdoor unit status
  SPECIAL_MODE = 247,
};

const MODE ClimateModeToInt(climate::ClimateMode mode);
const climate::ClimateMode IntToClimateMode(MODE mode);

const uint8_t ClimateSwingModeToCommand(climate::ClimateSwingMode mode);
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
const char *FixedPositionName(uint8_t zero_based_index);

const optional<SPECIAL_MODE> PresetToSpecialMode(const char* preset);
const char* SpecialModeToPreset(SPECIAL_MODE mode);

const optional<climate::ClimatePreset> StringToClimatePreset(const char *preset);
const char* ClimatePresetToString(climate::ClimatePreset preset);
const optional<SPECIAL_MODE> ClimatePresetToSpecialMode(climate::ClimatePreset preset);
const optional<climate::ClimatePreset> SpecialModeToClimatePreset(SPECIAL_MODE mode);

}  // namespace toshiba_suzumi
}  // namespace esphome
