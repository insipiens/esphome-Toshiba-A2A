#pragma once

#include <cstdint>
#include <string>

namespace esphome {
namespace toshiba_a2a {

enum class ToshibaIndoorUnitFamily : uint8_t {
  UNKNOWN = 0,
  J2FVG,
  P2KVSG,
};

enum class ToshibaHvacMode : uint8_t {
  UNKNOWN = 0,
  AUTO,
  COOL,
  HEAT,
  DRY,
  FAN,
};

// A3 is the only currently proven family-dependent command encoding.
enum class ToshibaLouvreEncoding : uint8_t {
  UNKNOWN = 0,
  J2_VERTICAL,
  P2_PACKED,
};

enum ToshibaFeature : uint32_t {
  FEATURE_NONE = 0,
  FEATURE_COMMON_HVAC = 1UL << 0,
  FEATURE_ECO = 1UL << 1,
  FEATURE_HI_POWER = 1UL << 2,
  FEATURE_COMFORT_SLEEP = 1UL << 3,
  FEATURE_POWER_SELECT = 1UL << 4,
  FEATURE_OUTDOOR_SILENT = 1UL << 5,
  FEATURE_FIREPLACE = 1UL << 6,
  FEATURE_EIGHT_DEG_HEAT = 1UL << 7,
  FEATURE_VERTICAL_AIRFLOW = 1UL << 8,
  FEATURE_HORIZONTAL_AIRFLOW = 1UL << 9,
  FEATURE_FLOOR = 1UL << 10,
  FEATURE_AIR_OUTLET_SELECT = 1UL << 11,
  FEATURE_HADA_CARE = 1UL << 12,
  FEATURE_SLEEP = 1UL << 13,
  FEATURE_COMFORT = 1UL << 14,
  FEATURE_PURE = 1UL << 15,
  FEATURE_START_DEFROST = 1UL << 16,
};

enum class ToshibaFeatureScope : uint8_t {
  UNKNOWN = 0,
  IDU_LOCAL,
  IDU_DEMAND,
  SHARED_ODU,
};

enum ToshibaFanOption : uint8_t {
  FAN_OPTION_NONE = 0,
  FAN_OPTION_MANUAL = 1U << 0,
  FAN_OPTION_AUTO = 1U << 1,
  FAN_OPTION_QUIET = 1U << 2,
};

struct ToshibaCapabilityProfile {
  uint32_t features{FEATURE_NONE};

  bool has(ToshibaFeature feature) const {
    return (features & static_cast<uint32_t>(feature)) != 0;
  }
};

ToshibaIndoorUnitFamily indoor_unit_family_from_model(const std::string &model);
const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family);
ToshibaCapabilityProfile capability_profile_from_model(const std::string &model);
ToshibaLouvreEncoding louvre_encoding_for_family(ToshibaIndoorUnitFamily family);
bool has_validated_mode_profile(ToshibaIndoorUnitFamily family);
bool validated_strong_defrost_allowed(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode);
ToshibaFeatureScope feature_scope(ToshibaFeature feature);
const char *feature_scope_to_string(ToshibaFeatureScope scope);
ToshibaCapabilityProfile validated_function_profile_for_mode(ToshibaIndoorUnitFamily family,
                                                              ToshibaHvacMode mode);
uint8_t validated_fan_options_for_mode(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode);

}  // namespace toshiba_a2a
}  // namespace esphome
