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
  FEATURE_COMFORT_SLEEP = 1UL << 3,  // separate 0x94 function; relationship to F7 Comfort still to be tested
  FEATURE_POWER_SELECT = 1UL << 4,
  FEATURE_OUTDOOR_SILENT = 1UL << 5,
  FEATURE_FIREPLACE = 1UL << 6,
  FEATURE_EIGHT_DEG_HEAT = 1UL << 7,
  FEATURE_VERTICAL_AIRFLOW = 1UL << 8,
  FEATURE_HORIZONTAL_AIRFLOW = 1UL << 9,
  FEATURE_FLOOR = 1UL << 10,
  FEATURE_AIR_OUTLET_SELECT = 1UL << 11,
  FEATURE_HADA_CARE = 1UL << 12,
  FEATURE_SLEEP = 1UL << 13,         // F7 Sleep
  FEATURE_COMFORT = 1UL << 14,       // F7 Comfort
  FEATURE_PURE = 1UL << 15,
  FEATURE_START_DEFROST = 1UL << 16, // momentary Function action; observed as CB 02 on P2KVSG
};

// User-facing scope classification. This is deliberately about control effect,
// not UART register locality:
//   IDU_LOCAL  - acts on local fan/airflow/air-treatment hardware.
//   IDU_DEMAND - changes this room's demand strategy; shared ODU may respond.
//   SHARED_ODU - directly constrains/commands the shared outdoor system.
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

  bool idu_model_available{false};
  bool odu_model_available{false};
  std::string idu_model;
  std::string idu_identity_1;
  std::string idu_identity_2;
  std::string idu_identity_3;
  std::string odu_model;
  std::string odu_identity_1;
  std::string odu_identity_2;
  std::string odu_identity_3;
  ToshibaIndoorUnitFamily idu_family{ToshibaIndoorUnitFamily::UNKNOWN};
  ToshibaCapabilityProfile capabilities;
};



ToshibaIndoorUnitFamily indoor_unit_family_from_model(const std::string &model);
const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family);
ToshibaCapabilityProfile capability_profile_from_model(const std::string &model);
ToshibaLouvreEncoding louvre_encoding_for_family(ToshibaIndoorUnitFamily family);
bool has_validated_mode_profile(ToshibaIndoorUnitFamily family);
bool validated_strong_defrost_allowed(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode);
ToshibaFeatureScope feature_scope(ToshibaFeature feature);
const char *feature_scope_to_string(ToshibaFeatureScope scope);

// These helpers deliberately contain only mode rules supported by Toshiba
// documentation and/or direct observation on the installed reference families.
// An empty result means "not validated for this family/mode", not "unsupported".
ToshibaCapabilityProfile validated_function_profile_for_mode(ToshibaIndoorUnitFamily family,
                                                              ToshibaHvacMode mode);
uint8_t validated_fan_options_for_mode(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode);
ToshibaCapabilityProfile power_select_cancel_profile(ToshibaIndoorUnitFamily family,
                                                      ToshibaHvacMode mode);

}  // namespace toshiba_a2a
}  // namespace esphome
