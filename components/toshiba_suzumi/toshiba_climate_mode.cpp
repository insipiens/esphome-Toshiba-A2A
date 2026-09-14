#include "toshiba_climate_mode.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "toshiba_climate.h"

namespace esphome {
namespace toshiba_suzumi {

const MODE ClimateModeToInt(climate::ClimateMode mode) {
  switch (mode) {
    case climate::CLIMATE_MODE_HEAT_COOL:
      return MODE::HEAT_COOL;
    case climate::CLIMATE_MODE_COOL:
      return MODE::COOL;
    case climate::CLIMATE_MODE_HEAT:
      return MODE::HEAT;
    case climate::CLIMATE_MODE_DRY:
      return MODE::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return MODE::FAN_ONLY;
    default:
      ESP_LOGE(TAG, "Invalid climate mode.");
      return MODE::HEAT_COOL;
  }
}

const climate::ClimateMode IntToClimateMode(MODE mode) {
  switch (mode) {
    case MODE::HEAT_COOL:
      return climate::CLIMATE_MODE_HEAT_COOL;
    case MODE::COOL:
      return climate::CLIMATE_MODE_COOL;
    case MODE::HEAT:
      return climate::CLIMATE_MODE_HEAT;
    case MODE::DRY:
      return climate::CLIMATE_MODE_DRY;
    case MODE::FAN_ONLY:
      return climate::CLIMATE_MODE_FAN_ONLY;
    default:
      ESP_LOGE(TAG, "Invalid climate mode.");
      return climate::CLIMATE_MODE_OFF;
  }
}

const optional<FAN> StringToFanLevel(const char* mode) {
  if (mode == CUSTOM_FAN_LEVEL_2) {
    return FAN::FANMODE_2;
  } else if (mode == CUSTOM_FAN_LEVEL_4) {
    return FAN::FANMODE_4;
  } else {
    return nullopt;
  }
}

const char* IntToCustomFanMode(FAN mode) {
  switch (mode) {
    case FAN::FANMODE_2:
      return CUSTOM_FAN_LEVEL_2;
    case FAN::FANMODE_4:
      return CUSTOM_FAN_LEVEL_4;
    default:
      return "Unknown";
  }
}

const optional<PWR_LEVEL> StringToPwrLevel(const std::string &mode) {
  if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_100)) {
    return PWR_LEVEL::PCT_100;
  } else if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_75)) {
    return PWR_LEVEL::PCT_75;
  } else if (str_equals_case_insensitive(mode, CUSTOM_PWR_LEVEL_50)) {
    return PWR_LEVEL::PCT_50;
  } else {
    return nullopt;
  }
}

const std::string IntToPowerLevel(PWR_LEVEL mode) {
  switch (mode) {
    case PWR_LEVEL::PCT_100:
      return CUSTOM_PWR_LEVEL_100;
    case PWR_LEVEL::PCT_75:
      return CUSTOM_PWR_LEVEL_75;
    case PWR_LEVEL::PCT_50:
      return CUSTOM_PWR_LEVEL_50;
    default:
      return "Unknown";
  }
}

struct VerticalAirDirection {
  SWING swing;
  const char *name;
};

static const char *const FIXED_POSITION_NAMES[] = {
    "Position 1", "Position 2", "Position 3", "Position 4", "Position 5", "Position 6"};

// Toshiba service manuals call the physical up/down flap a horizontal louver;
// expose the user-facing effect as vertical air direction.  The six fixed
// values are the genuine RB-N106S-G writes captured during the ordered P2
// vertical FIX sweep.  The old inherited 0x50..0x54 mapping is no longer used.
static const VerticalAirDirection VERTICAL_AIR_DIRECTIONS[] = {
    {SWING::OFF, "Off"},
    {SWING::VERTICAL, "Swing"},
    {SWING::VERTICAL_FIX_POSITION_1, "Position 1"},
    {SWING::VERTICAL_FIX_POSITION_2, "Position 2"},
    {SWING::VERTICAL_FIX_POSITION_3, "Position 3"},
    {SWING::VERTICAL_FIX_POSITION_4, "Position 4"},
    {SWING::VERTICAL_FIX_POSITION_5, "Position 5"},
    {SWING::VERTICAL_FIX_POSITION_6, "Position 6"},
};

const char *FixedPositionName(uint8_t zero_based_index) {
  if (zero_based_index > 5) return nullptr;
  return FIXED_POSITION_NAMES[zero_based_index];
}

bool DecodePackedFixPosition(uint8_t raw, uint8_t &horizontal_index, uint8_t &vertical_index) {
  // Controlled Toshiba-app captures showed:
  //   vertical sweep:   88 89 8A 8B 8C 8D
  //   horizontal sweep: 85 8D 95 9D A5 AD
  // This is a packed 3-bit + 3-bit position structure with the top bit set.
  if ((raw & 0xC0) != 0x80) return false;
  horizontal_index = (raw & 0x38) >> 3;
  vertical_index = raw & 0x07;
  return horizontal_index <= 5 && vertical_index <= 5;
}

const optional<SWING> StringToVerticalAirDirection(const std::string &position) {
  for (auto const &direction : VERTICAL_AIR_DIRECTIONS) {
    if (str_equals_case_insensitive(position, direction.name)) {
      return direction.swing;
    }
  }

  // Backwards-compatible aliases for older YAML/UI names. They now resolve to
  // observed P2 codes instead of the superseded inherited 0x50..0x54 values.
  if (str_equals_case_insensitive(position, "Top")) return SWING::VERTICAL_FIX_POSITION_1;
  if (str_equals_case_insensitive(position, "Middle Top")) return SWING::VERTICAL_FIX_POSITION_2;
  if (str_equals_case_insensitive(position, "Middle")) return SWING::VERTICAL_FIX_POSITION_3;
  if (str_equals_case_insensitive(position, "Middle Bottom")) return SWING::VERTICAL_FIX_POSITION_5;
  if (str_equals_case_insensitive(position, "Bottom")) return SWING::VERTICAL_FIX_POSITION_6;
  return nullopt;
}

const char* SwingToVerticalAirDirection(SWING mode) {
  if (mode == SWING::HORIZONTAL) {
    return "Off";
  }
  if (mode == SWING::BOTH) {
    return "Swing";
  }

  uint8_t horizontal_index = 0;
  uint8_t vertical_index = 0;
  if (DecodePackedFixPosition(static_cast<uint8_t>(mode), horizontal_index, vertical_index)) {
    return FixedPositionName(vertical_index);
  }

  for (auto const &direction : VERTICAL_AIR_DIRECTIONS) {
    if (mode == direction.swing) {
      return direction.name;
    }
  }
  return nullptr;
}

bool IsFixedVerticalAirDirection(SWING mode) {
  uint8_t horizontal_index = 0;
  uint8_t vertical_index = 0;
  return DecodePackedFixPosition(static_cast<uint8_t>(mode), horizontal_index, vertical_index);
}

const SWING ClimateSwingModeToInt(climate::ClimateSwingMode mode) {
  switch (mode) {
    case climate::CLIMATE_SWING_OFF:
      return SWING::OFF;
    case climate::CLIMATE_SWING_BOTH:
      return SWING::BOTH;
    case climate::CLIMATE_SWING_VERTICAL:
      return SWING::VERTICAL;
    case climate::CLIMATE_SWING_HORIZONTAL:
      return SWING::HORIZONTAL;
    default:
      ESP_LOGE(TAG, "Invalid swing mode %d.", mode);
      return SWING::OFF;
  }
}

const climate::ClimateSwingMode IntToClimateSwingMode(SWING mode) {
  switch (mode) {
    case SWING::OFF:
      return climate::CLIMATE_SWING_OFF;
    case SWING::VERTICAL:
      return climate::CLIMATE_SWING_VERTICAL;
    case SWING::HORIZONTAL:
      return climate::CLIMATE_SWING_HORIZONTAL;
    case SWING::BOTH:
      return climate::CLIMATE_SWING_BOTH;
    case SWING::HADA:
      return climate::CLIMATE_SWING_OFF;
    default:
      return climate::CLIMATE_SWING_OFF;
  }
}

const optional<FAN> ClimateFanModeToInt(climate::ClimateFanMode mode) {
  switch (mode) {
    case climate::CLIMATE_FAN_AUTO:
      return FAN::FAN_AUTO;
    case climate::CLIMATE_FAN_QUIET:
      return FAN::FAN_QUIET;
    case climate::CLIMATE_FAN_LOW:
      return FAN::FAN_LOW;
    case climate::CLIMATE_FAN_MEDIUM:
      return FAN::FAN_MEDIUM;
    case climate::CLIMATE_FAN_HIGH:
      return FAN::FAN_HIGH;
    default:
      return nullopt;
  }
}

const LogString *climate_state_to_string(STATE mode) {
  switch (mode) {
    case STATE::ON:
      return LOG_STR("ON");
    case STATE::OFF:
      return LOG_STR("OFF");
    default:
      return LOG_STR("UNKNOWN");
  }
}

const optional<SPECIAL_MODE> PresetToSpecialMode(const char* preset) {
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_STANDARD)) {
    return SPECIAL_MODE::STANDARD;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_HI_POWER)) {
    return SPECIAL_MODE::HI_POWER;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_ECO)) {
    return SPECIAL_MODE::ECO;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_FIREPLACE_1)) {
    return SPECIAL_MODE::FIREPLACE_1;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_FIREPLACE_2)) {
    return SPECIAL_MODE::FIREPLACE_2;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_EIGHT_DEG)) {
    return SPECIAL_MODE::EIGHT_DEG;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_SILENT_1)) {
    return SPECIAL_MODE::SILENT_1;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_SILENT_2)) {
    return SPECIAL_MODE::SILENT_2;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_SLEEP)) {
    return SPECIAL_MODE::SLEEP;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_FLOOR)) {
    return SPECIAL_MODE::FLOOR;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_COMFORT)) {
    return SPECIAL_MODE::COMFORT;
  } else {
    return nullopt;
  }
}

const char* SpecialModeToPreset(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::STANDARD:
      return SPECIAL_MODE_STANDARD;
    case SPECIAL_MODE::HI_POWER:
      return SPECIAL_MODE_HI_POWER;
    case SPECIAL_MODE::ECO:
      return SPECIAL_MODE_ECO;
    case SPECIAL_MODE::FIREPLACE_1:
      return SPECIAL_MODE_FIREPLACE_1;
    case SPECIAL_MODE::FIREPLACE_2:
      return SPECIAL_MODE_FIREPLACE_2;
    case SPECIAL_MODE::EIGHT_DEG:
      return SPECIAL_MODE_EIGHT_DEG;
    case SPECIAL_MODE::SILENT_1:
      return SPECIAL_MODE_SILENT_1;
    case SPECIAL_MODE::SILENT_2:
      return SPECIAL_MODE_SILENT_2;
    case SPECIAL_MODE::SLEEP:
      return SPECIAL_MODE_SLEEP;
    case SPECIAL_MODE::FLOOR:
      return SPECIAL_MODE_FLOOR;
    case SPECIAL_MODE::COMFORT:
      return SPECIAL_MODE_COMFORT;
    default:
      return SPECIAL_MODE_STANDARD;
  }
}

const optional<climate::ClimatePreset> StringToClimatePreset(const char *preset) {
  if (str_equals_case_insensitive(preset, SPECIAL_MODE_STANDARD)) {
    return climate::CLIMATE_PRESET_NONE;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_ECO)) {
    return climate::CLIMATE_PRESET_ECO;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_HI_POWER)) {
    return climate::CLIMATE_PRESET_BOOST;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_SLEEP)) {
    return climate::CLIMATE_PRESET_SLEEP;
  } else if (str_equals_case_insensitive(preset, SPECIAL_MODE_COMFORT)) {
    return climate::CLIMATE_PRESET_COMFORT;
  }
  return nullopt;
}

const char* ClimatePresetToString(climate::ClimatePreset preset) {
  switch (preset) {
    case climate::CLIMATE_PRESET_NONE:
      return SPECIAL_MODE_STANDARD;
    case climate::CLIMATE_PRESET_ECO:
      return SPECIAL_MODE_ECO;
    case climate::CLIMATE_PRESET_BOOST:
      return SPECIAL_MODE_HI_POWER;
    case climate::CLIMATE_PRESET_SLEEP:
      return SPECIAL_MODE_SLEEP;
    case climate::CLIMATE_PRESET_COMFORT:
      return SPECIAL_MODE_COMFORT;
    default:
      return SPECIAL_MODE_STANDARD;
  }
}

const optional<SPECIAL_MODE> ClimatePresetToSpecialMode(climate::ClimatePreset preset) {
  auto preset_string = ClimatePresetToString(preset);
  return PresetToSpecialMode(preset_string);
}

const optional<climate::ClimatePreset> SpecialModeToClimatePreset(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::STANDARD:
      return climate::CLIMATE_PRESET_NONE;
    case SPECIAL_MODE::ECO:
      return climate::CLIMATE_PRESET_ECO;
    case SPECIAL_MODE::HI_POWER:
      return climate::CLIMATE_PRESET_BOOST;
    case SPECIAL_MODE::SLEEP:
      return climate::CLIMATE_PRESET_SLEEP;
    case SPECIAL_MODE::COMFORT:
      return climate::CLIMATE_PRESET_COMFORT;
    default:
      return nullopt;
  }
}

}  // namespace toshiba_suzumi
}  // namespace esphome
