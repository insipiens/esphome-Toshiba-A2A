#include "toshiba_climate_mode.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "toshiba_climate.h"

namespace esphome {
namespace toshiba_suzumi {

const MODE ClimateModeToInt(climate::ClimateMode mode) {
  switch (mode) {
    case climate::CLIMATE_MODE_HEAT_COOL: return MODE::HEAT_COOL;
    case climate::CLIMATE_MODE_COOL: return MODE::COOL;
    case climate::CLIMATE_MODE_HEAT: return MODE::HEAT;
    case climate::CLIMATE_MODE_DRY: return MODE::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY: return MODE::FAN_ONLY;
    default:
      ESP_LOGE(TAG, "Invalid climate mode.");
      return MODE::HEAT_COOL;
  }
}

const climate::ClimateMode IntToClimateMode(MODE mode) {
  switch (mode) {
    case MODE::HEAT_COOL: return climate::CLIMATE_MODE_HEAT_COOL;
    case MODE::COOL: return climate::CLIMATE_MODE_COOL;
    case MODE::HEAT: return climate::CLIMATE_MODE_HEAT;
    case MODE::DRY: return climate::CLIMATE_MODE_DRY;
    case MODE::FAN_ONLY: return climate::CLIMATE_MODE_FAN_ONLY;
    default:
      ESP_LOGE(TAG, "Invalid climate mode.");
      return climate::CLIMATE_MODE_OFF;
  }
}

const optional<FAN> StringToFanLevel(const char* mode) {
  if (mode == CUSTOM_FAN_LEVEL_2) return FAN::FANMODE_2;
  if (mode == CUSTOM_FAN_LEVEL_4) return FAN::FANMODE_4;
  return nullopt;
}

const char* IntToCustomFanMode(FAN mode) {
  switch (mode) {
    case FAN::FANMODE_2: return CUSTOM_FAN_LEVEL_2;
    case FAN::FANMODE_4: return CUSTOM_FAN_LEVEL_4;
    default: return "Unknown";
  }
}

const optional<PWR_LEVEL> StringToPwrLevel(const std::string &mode) {
  if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_100)) return PWR_LEVEL::PCT_100;
  if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_75)) return PWR_LEVEL::PCT_75;
  if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_50)) return PWR_LEVEL::PCT_50;
  return nullopt;
}

const std::string IntToPowerLevel(PWR_LEVEL mode) {
  switch (mode) {
    case PWR_LEVEL::PCT_100: return CUSTOM_PWR_LEVEL_100;
    case PWR_LEVEL::PCT_75: return CUSTOM_PWR_LEVEL_75;
    case PWR_LEVEL::PCT_50: return CUSTOM_PWR_LEVEL_50;
    default: return "Unknown";
  }
}

struct VerticalAirDirection {
  SWING swing;
  const char *name;
};

static const char *const LEGACY_FIXED_POSITION_NAMES[] = {
    "Position 1", "Position 2", "Position 3", "Position 4", "Position 5"};
static const char *const VERTICAL_FIXED_POSITION_NAMES[] = {
    "Top", "Upper", "Centre", "Lower", "Bottom"};
static const char *const HORIZONTAL_FIXED_POSITION_NAMES[] = {
    "Left", "Left-Centre", "Centre", "Right-Centre", "Right"};

static const VerticalAirDirection VERTICAL_AIR_DIRECTIONS[] = {
    {SWING::OFF, "Off"},
    {SWING::VERTICAL, "Swing"},
    {SWING::VERTICAL_FIX_POSITION_1, "Top"},
    {SWING::VERTICAL_FIX_POSITION_2, "Upper"},
    {SWING::VERTICAL_FIX_POSITION_3, "Centre"},
    {SWING::VERTICAL_FIX_POSITION_4, "Lower"},
    {SWING::VERTICAL_FIX_POSITION_5, "Bottom"},
};

const char *FixedPositionName(uint8_t position_index) {
  if (position_index < 1 || position_index > 5) return nullptr;
  return LEGACY_FIXED_POSITION_NAMES[position_index - 1];
}

const char *VerticalFixedPositionName(uint8_t position_index) {
  if (position_index < 1 || position_index > 5) return nullptr;
  return VERTICAL_FIXED_POSITION_NAMES[position_index - 1];
}

const char *HorizontalFixedPositionName(uint8_t position_index) {
  if (position_index < 1 || position_index > 5) return nullptr;
  return HORIZONTAL_FIXED_POSITION_NAMES[position_index - 1];
}

const optional<uint8_t> FixedPositionIndexFromName(const std::string &value) {
  for (uint8_t index = 1; index <= 5; index++) {
    const char *legacy = FixedPositionName(index);
    const char *vertical = VerticalFixedPositionName(index);
    const char *horizontal = HorizontalFixedPositionName(index);
    if ((legacy != nullptr && str_equals_case_insensitive(value, legacy)) ||
        (vertical != nullptr && str_equals_case_insensitive(value, vertical)) ||
        (horizontal != nullptr && str_equals_case_insensitive(value, horizontal))) {
      return index;
    }
  }
  return nullopt;
}

bool DecodePackedFixPosition(uint8_t raw, uint8_t &horizontal_index, uint8_t &vertical_index) {
  // A3 FIX byte:
  //   bit 7     = FIX flag
  //   bits 5..3 = horizontal field
  //   bits 2..0 = vertical field
  // Controlled captures exercise field values 0..5. The HA controls expose
  // five indexed user positions (1..5); raw field 0 remains observable but is
  // not assigned a physical label.
  if ((raw & 0xC0) != 0x80) return false;
  horizontal_index = (raw >> 3) & 0x07;
  vertical_index = raw & 0x07;
  return horizontal_index <= 5 && vertical_index <= 5;
}

uint8_t EncodePackedFixPosition(uint8_t horizontal_index, uint8_t vertical_index) {
  horizontal_index &= 0x07;
  vertical_index &= 0x07;
  return static_cast<uint8_t>(0x80 | (horizontal_index << 3) | vertical_index);
}

const optional<SWING> StringToVerticalAirDirection(const std::string &position) {
  for (auto const &direction : VERTICAL_AIR_DIRECTIONS) {
    if (str_equals_case_insensitive(position, direction.name)) return direction.swing;
  }

  // Preserve compatibility with configurations/entities created while the
  // positions were exposed as generic numeric labels.
  if (str_equals_case_insensitive(position, "Position 1")) return SWING::VERTICAL_FIX_POSITION_1;
  if (str_equals_case_insensitive(position, "Position 2")) return SWING::VERTICAL_FIX_POSITION_2;
  if (str_equals_case_insensitive(position, "Position 3")) return SWING::VERTICAL_FIX_POSITION_3;
  if (str_equals_case_insensitive(position, "Position 4")) return SWING::VERTICAL_FIX_POSITION_4;
  if (str_equals_case_insensitive(position, "Position 5")) return SWING::VERTICAL_FIX_POSITION_5;
  return nullopt;
}

const char* SwingToVerticalAirDirection(SWING mode) {
  if (mode == SWING::HORIZONTAL) return "Off";
  if (mode == SWING::BOTH) return "Swing";

  uint8_t horizontal_index = 0;
  uint8_t vertical_index = 0;
  if (DecodePackedFixPosition(static_cast<uint8_t>(mode), horizontal_index, vertical_index)) {
    return VerticalFixedPositionName(vertical_index);
  }

  for (auto const &direction : VERTICAL_AIR_DIRECTIONS) {
    if (mode == direction.swing) return direction.name;
  }
  return nullptr;
}

bool IsFixedVerticalAirDirection(SWING mode) {
  uint8_t horizontal_index = 0;
  uint8_t vertical_index = 0;
  return DecodePackedFixPosition(static_cast<uint8_t>(mode), horizontal_index, vertical_index);
}

const uint8_t ClimateSwingModeToCommand(climate::ClimateSwingMode mode) {
  switch (mode) {
    case climate::CLIMATE_SWING_OFF: return A3_CMD_OFF_TRANSITION;
    case climate::CLIMATE_SWING_BOTH: return A3_CMD_BOTH_SWING;
    case climate::CLIMATE_SWING_VERTICAL: return A3_CMD_VERTICAL_SWING;
    case climate::CLIMATE_SWING_HORIZONTAL: return A3_CMD_HORIZONTAL_SWING;
    default:
      ESP_LOGE(TAG, "Invalid swing mode %d.", mode);
      return A3_CMD_OFF_TRANSITION;
  }
}

const climate::ClimateSwingMode IntToClimateSwingMode(SWING mode) {
  switch (mode) {
    case SWING::OFF: return climate::CLIMATE_SWING_OFF;
    case SWING::VERTICAL: return climate::CLIMATE_SWING_VERTICAL;
    case SWING::HORIZONTAL: return climate::CLIMATE_SWING_HORIZONTAL;
    case SWING::BOTH: return climate::CLIMATE_SWING_BOTH;
    case SWING::HADA: return climate::CLIMATE_SWING_OFF;
    default: return climate::CLIMATE_SWING_OFF;
  }
}

const optional<FAN> ClimateFanModeToInt(climate::ClimateFanMode mode) {
  switch (mode) {
    case climate::CLIMATE_FAN_AUTO: return FAN::FAN_AUTO;
    case climate::CLIMATE_FAN_QUIET: return FAN::FAN_QUIET;
    case climate::CLIMATE_FAN_LOW: return FAN::FAN_LOW;
    case climate::CLIMATE_FAN_MEDIUM: return FAN::FAN_MEDIUM;
    case climate::CLIMATE_FAN_HIGH: return FAN::FAN_HIGH;
    default: return nullopt;
  }
}

const LogString *climate_state_to_string(STATE mode) {
  switch (mode) {
    case STATE::ON: return LOG_STR("ON");
    case STATE::OFF: return LOG_STR("OFF");
    default: return LOG_STR("UNKNOWN");
  }
}

const optional<SPECIAL_MODE> PresetToSpecialMode(const char* preset) {
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_STANDARD)) return SPECIAL_MODE::STANDARD;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_HI_POWER)) return SPECIAL_MODE::HI_POWER;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_ECO)) return SPECIAL_MODE::ECO;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_FIREPLACE_1)) return SPECIAL_MODE::FIREPLACE_1;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_FIREPLACE_2)) return SPECIAL_MODE::FIREPLACE_2;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_EIGHT_DEG)) return SPECIAL_MODE::EIGHT_DEG;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_SILENT_1)) return SPECIAL_MODE::SILENT_1;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_SILENT_2)) return SPECIAL_MODE::SILENT_2;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_SLEEP)) return SPECIAL_MODE::SLEEP;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_FLOOR)) return SPECIAL_MODE::FLOOR;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_COMFORT)) return SPECIAL_MODE::COMFORT;
  return nullopt;
}

const char* SpecialModeToPreset(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::STANDARD: return SPECIAL_MODE_STANDARD;
    case SPECIAL_MODE::HI_POWER: return SPECIAL_MODE_HI_POWER;
    case SPECIAL_MODE::ECO: return SPECIAL_MODE_ECO;
    case SPECIAL_MODE::FIREPLACE_1: return SPECIAL_MODE_FIREPLACE_1;
    case SPECIAL_MODE::FIREPLACE_2: return SPECIAL_MODE_FIREPLACE_2;
    case SPECIAL_MODE::EIGHT_DEG: return SPECIAL_MODE_EIGHT_DEG;
    case SPECIAL_MODE::SILENT_1: return SPECIAL_MODE_SILENT_1;
    case SPECIAL_MODE::SILENT_2: return SPECIAL_MODE_SILENT_2;
    case SPECIAL_MODE::SLEEP: return SPECIAL_MODE_SLEEP;
    case SPECIAL_MODE::FLOOR: return SPECIAL_MODE_FLOOR;
    case SPECIAL_MODE::COMFORT: return SPECIAL_MODE_COMFORT;
    default: return SPECIAL_MODE_STANDARD;
  }
}

const optional<climate::ClimatePreset> StringToClimatePreset(const char *preset) {
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_STANDARD)) return climate::CLIMATE_PRESET_NONE;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_ECO)) return climate::CLIMATE_PRESET_ECO;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_HI_POWER)) return climate::CLIMATE_PRESET_BOOST;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_SLEEP)) return climate::CLIMATE_PRESET_SLEEP;
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_COMFORT)) return climate::CLIMATE_PRESET_COMFORT;
  return nullopt;
}

const char* ClimatePresetToString(climate::ClimatePreset preset) {
  switch (preset) {
    case climate::CLIMATE_PRESET_NONE: return SPECIAL_MODE_STANDARD;
    case climate::CLIMATE_PRESET_ECO: return SPECIAL_MODE_ECO;
    case climate::CLIMATE_PRESET_BOOST: return SPECIAL_MODE_HI_POWER;
    case climate::CLIMATE_PRESET_SLEEP: return SPECIAL_MODE_SLEEP;
    case climate::CLIMATE_PRESET_COMFORT: return SPECIAL_MODE_COMFORT;
    default: return SPECIAL_MODE_STANDARD;
  }
}

const optional<SPECIAL_MODE> ClimatePresetToSpecialMode(climate::ClimatePreset preset) {
  return PresetToSpecialMode(ClimatePresetToString(preset));
}

const optional<climate::ClimatePreset> SpecialModeToClimatePreset(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::STANDARD: return climate::CLIMATE_PRESET_NONE;
    case SPECIAL_MODE::ECO: return climate::CLIMATE_PRESET_ECO;
    case SPECIAL_MODE::HI_POWER: return climate::CLIMATE_PRESET_BOOST;
    case SPECIAL_MODE::SLEEP: return climate::CLIMATE_PRESET_SLEEP;
    case SPECIAL_MODE::COMFORT: return climate::CLIMATE_PRESET_COMFORT;
    default: return nullopt;
  }
}

}  // namespace toshiba_suzumi
}  // namespace esphome
