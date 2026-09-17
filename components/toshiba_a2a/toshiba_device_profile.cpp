#include "toshiba_device_profile.h"

namespace esphome {
namespace toshiba_a2a {

namespace {

constexpr uint32_t COMMON_RESIDENTIAL_FEATURES =
    FEATURE_COMMON_HVAC |
    FEATURE_ECO |
    FEATURE_HI_POWER |
    FEATURE_COMFORT_SLEEP |
    FEATURE_POWER_SELECT |
    FEATURE_OUTDOOR_SILENT |
    FEATURE_FIREPLACE |
    FEATURE_EIGHT_DEG_HEAT |
    FEATURE_VERTICAL_AIRFLOW |
    FEATURE_SLEEP |
    FEATURE_COMFORT;

constexpr uint32_t J2FVG_FEATURES =
    FEATURE_COMMON_HVAC |
    FEATURE_ECO |
    FEATURE_HI_POWER |
    FEATURE_COMFORT_SLEEP |
    FEATURE_POWER_SELECT |
    FEATURE_OUTDOOR_SILENT |
    FEATURE_FIREPLACE |
    FEATURE_EIGHT_DEG_HEAT |
    FEATURE_VERTICAL_AIRFLOW |
    FEATURE_FLOOR |
    FEATURE_AIR_OUTLET_SELECT;

struct ToshibaFamilyCapabilityEntry {
  ToshibaIndoorUnitFamily family;
  uint32_t features;
};

// Family capability matrix: functionality belongs to the indoor-unit family,
// not to the capacity variant. Exact-model quantitative data such as airflow is
// kept separately in the output component.
static constexpr ToshibaFamilyCapabilityEntry TOSHIBA_FAMILY_CAPABILITIES[] = {
    {ToshibaIndoorUnitFamily::J2FVG, J2FVG_FEATURES},
    {ToshibaIndoorUnitFamily::P2KVSG,
     COMMON_RESIDENTIAL_FEATURES |
         FEATURE_HORIZONTAL_AIRFLOW |
         FEATURE_PURE |
         FEATURE_START_DEFROST},
};

struct ToshibaModeFunctionEntry {
  ToshibaIndoorUnitFamily family;
  ToshibaHvacMode mode;
  uint32_t features;
  uint8_t fan_options;
};

// Mode matrix is intentionally evidence-limited. J2 entries come from the
// Toshiba RAS-B10/B13/B18J2FVG-E operation/service documentation plus direct
// observation of the installed J2 remote/app behaviour. P2 entries come from
// the genuine Toshiba app on the directly tested RAS-B10P2KVSGB-E.
static constexpr ToshibaModeFunctionEntry TOSHIBA_VALIDATED_MODE_FUNCTIONS[] = {
    {ToshibaIndoorUnitFamily::J2FVG, ToshibaHvacMode::AUTO,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_COMFORT_SLEEP,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::J2FVG, ToshibaHvacMode::COOL,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_COMFORT_SLEEP,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::J2FVG, ToshibaHvacMode::HEAT,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_COMFORT_SLEEP |
         FEATURE_FIREPLACE | FEATURE_EIGHT_DEG_HEAT | FEATURE_FLOOR,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::J2FVG, ToshibaHvacMode::DRY,
     FEATURE_POWER_SELECT,
     FAN_OPTION_AUTO},
    {ToshibaIndoorUnitFamily::J2FVG, ToshibaHvacMode::FAN,
     FEATURE_POWER_SELECT,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},

    {ToshibaIndoorUnitFamily::P2KVSG, ToshibaHvacMode::AUTO,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE | FEATURE_START_DEFROST,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::P2KVSG, ToshibaHvacMode::COOL,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::P2KVSG, ToshibaHvacMode::HEAT,
     FEATURE_POWER_SELECT | FEATURE_ECO | FEATURE_HI_POWER |
         FEATURE_OUTDOOR_SILENT | FEATURE_PURE | FEATURE_EIGHT_DEG_HEAT |
         FEATURE_START_DEFROST,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
    {ToshibaIndoorUnitFamily::P2KVSG, ToshibaHvacMode::DRY,
     FEATURE_POWER_SELECT | FEATURE_PURE,
     FAN_OPTION_AUTO},
    {ToshibaIndoorUnitFamily::P2KVSG, ToshibaHvacMode::FAN,
     FEATURE_POWER_SELECT | FEATURE_PURE,
     FAN_OPTION_MANUAL | FAN_OPTION_AUTO | FAN_OPTION_QUIET},
};

}  // namespace

ToshibaIndoorUnitFamily indoor_unit_family_from_model(const std::string &model) {
  if (model.find("J2FVG") != std::string::npos) return ToshibaIndoorUnitFamily::J2FVG;
  if (model.find("P2KVSG") != std::string::npos) return ToshibaIndoorUnitFamily::P2KVSG;
  return ToshibaIndoorUnitFamily::UNKNOWN;
}

const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family) {
  switch (family) {
    case ToshibaIndoorUnitFamily::J2FVG: return "J2FVG";
    case ToshibaIndoorUnitFamily::P2KVSG: return "P2KVSG";
    default: return "UNKNOWN";
  }
}

ToshibaFeatureScope feature_scope(ToshibaFeature feature) {
  switch (feature) {
    case FEATURE_VERTICAL_AIRFLOW:
    case FEATURE_HORIZONTAL_AIRFLOW:
    case FEATURE_FLOOR:
    case FEATURE_AIR_OUTLET_SELECT:
    case FEATURE_FIREPLACE:
    case FEATURE_HADA_CARE:
    case FEATURE_PURE:
      return ToshibaFeatureScope::IDU_LOCAL;

    case FEATURE_COMMON_HVAC:
    case FEATURE_ECO:
    case FEATURE_HI_POWER:
    case FEATURE_COMFORT_SLEEP:
    case FEATURE_EIGHT_DEG_HEAT:
    case FEATURE_SLEEP:
    case FEATURE_COMFORT:
      return ToshibaFeatureScope::IDU_DEMAND;

    case FEATURE_POWER_SELECT:
    case FEATURE_OUTDOOR_SILENT:
    case FEATURE_START_DEFROST:
      return ToshibaFeatureScope::SHARED_ODU;

    default:
      return ToshibaFeatureScope::UNKNOWN;
  }
}

const char *feature_scope_to_string(ToshibaFeatureScope scope) {
  switch (scope) {
    case ToshibaFeatureScope::IDU_LOCAL: return "IDU local";
    case ToshibaFeatureScope::IDU_DEMAND: return "IDU demand / shared consequence";
    case ToshibaFeatureScope::SHARED_ODU: return "Shared ODU";
    default: return "Unknown";
  }
}

ToshibaCapabilityProfile capability_profile_from_model(const std::string &model) {
  ToshibaCapabilityProfile profile;
  const auto family = indoor_unit_family_from_model(model);

  for (const auto &entry : TOSHIBA_FAMILY_CAPABILITIES) {
    if (entry.family == family) {
      profile.features = entry.features;
      return profile;
    }
  }

  profile.features = FEATURE_COMMON_HVAC;
  return profile;
}

ToshibaLouvreEncoding louvre_encoding_for_family(ToshibaIndoorUnitFamily family) {
  switch (family) {
    case ToshibaIndoorUnitFamily::J2FVG: return ToshibaLouvreEncoding::J2_VERTICAL;
    case ToshibaIndoorUnitFamily::P2KVSG: return ToshibaLouvreEncoding::P2_PACKED;
    default: return ToshibaLouvreEncoding::UNKNOWN;
  }
}

bool has_validated_mode_profile(ToshibaIndoorUnitFamily family) {
  for (const auto &entry : TOSHIBA_VALIDATED_MODE_FUNCTIONS) {
    if (entry.family == family) return true;
  }
  return false;
}

bool validated_strong_defrost_allowed(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode) {
  return family == ToshibaIndoorUnitFamily::P2KVSG && mode == ToshibaHvacMode::HEAT;
}

ToshibaCapabilityProfile validated_function_profile_for_mode(ToshibaIndoorUnitFamily family,
                                                              ToshibaHvacMode mode) {
  ToshibaCapabilityProfile profile;
  for (const auto &entry : TOSHIBA_VALIDATED_MODE_FUNCTIONS) {
    if (entry.family == family && entry.mode == mode) {
      profile.features = entry.features;
      break;
    }
  }
  return profile;
}

uint8_t validated_fan_options_for_mode(ToshibaIndoorUnitFamily family, ToshibaHvacMode mode) {
  for (const auto &entry : TOSHIBA_VALIDATED_MODE_FUNCTIONS) {
    if (entry.family == family && entry.mode == mode) return entry.fan_options;
  }
  return FAN_OPTION_NONE;
}



}  // namespace toshiba_a2a
}  // namespace esphome
