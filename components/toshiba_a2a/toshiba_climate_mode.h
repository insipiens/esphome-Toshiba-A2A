#pragma once

#include <cstdint>
#include "esphome/core/log.h"
#include "esphome/components/climate/climate.h"
#include "toshiba_registers.h"

namespace esphome {
namespace toshiba_a2a {

// Expose the complete Toshiba fan ladder as one ordered custom list so Home
// Assistant does not place ESPHome standard fan modes ahead of the intermediate
// Toshiba levels. Physical order is Auto, Quiet, Low, Low-Medium, Medium,
// Medium-High, High.
constexpr const char* CUSTOM_FAN_AUTO = "Auto";
constexpr const char* CUSTOM_FAN_QUIET = "Quiet";
constexpr const char* CUSTOM_FAN_LOW = "Low";
constexpr const char* CUSTOM_FAN_LEVEL_2 = "Low-Medium";
constexpr const char* CUSTOM_FAN_MEDIUM = "Medium";
constexpr const char* CUSTOM_FAN_LEVEL_4 = "Medium-High";
constexpr const char* CUSTOM_FAN_HIGH = "High";

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

using MODE=reg_b0::Mode; using FAN=reg_a0::Fan; using STATE=reg_80::State; using PWR_LEVEL=reg_87::PowerLevel;
using MAINTENANCE_STATE=reg_cb::State; using SELF_CLEAN_STATE=reg_cb::SelfCleanState; using SPECIAL_MODE=reg_f7::SpecialMode;

const MODE ClimateModeToInt(climate::ClimateMode mode);
const climate::ClimateMode IntToClimateMode(MODE mode);

const optional<FAN> ClimateFanModeToInt(climate::ClimateFanMode mode);
const LogString *climate_state_to_string(STATE mode);
const optional<FAN> StringToFanLevel(const char* mode);
const char* IntToCustomFanMode(FAN mode);
const optional<PWR_LEVEL> StringToPwrLevel(const std::string &mode);
const std::string IntToPowerLevel(PWR_LEVEL mode);

const char *FixedPositionName(uint8_t position_index);  // legacy numeric label
const char *VerticalFixedPositionName(uint8_t position_index);
const char *HorizontalFixedPositionName(uint8_t position_index);
const optional<uint8_t> FixedPositionIndexFromName(const std::string &value);

const optional<SPECIAL_MODE> PresetToSpecialMode(const char* preset);
const char* SpecialModeToPreset(SPECIAL_MODE mode);
const optional<climate::ClimatePreset> StringToClimatePreset(const char *preset);
const char* ClimatePresetToString(climate::ClimatePreset preset);
const optional<SPECIAL_MODE> ClimatePresetToSpecialMode(climate::ClimatePreset preset);
const optional<climate::ClimatePreset> SpecialModeToClimatePreset(SPECIAL_MODE mode);

}  // namespace toshiba_a2a
}  // namespace esphome