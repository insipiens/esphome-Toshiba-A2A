#include "toshiba_model.h"

#include <cctype>

namespace esphome {
namespace toshiba_suzumi {

namespace {

std::string decode_ascii_field(const std::vector<uint8_t> &raw_data, size_t offset, size_t width) {
  if (offset + width > raw_data.size()) return {};

  std::string value;
  value.reserve(width);
  for (size_t i = 0; i < width; i++) {
    const uint8_t byte = raw_data[offset + i];
    if (byte == 0x00 || byte == 0xFF) break;
    if (byte < 0x20 || byte > 0x7E) break;
    value.push_back(static_cast<char>(byte));
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
  return value;
}

bool is_model_field_available(const std::string &value) {
  return !value.empty() && value != "NULL" && value.rfind("RAS-", 0) == 0;
}

bool is_identity_field_available(const std::string &value) {
  return !value.empty() && value != "NULL";
}

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

struct ToshibaFamilyCapabilityEntry {
  ToshibaIndoorUnitFamily family;
  uint32_t features;
};

// Family capability matrix: functionality belongs to the indoor-unit family,
// not to the capacity variant. Exact-model quantitative data such as airflow is
// kept separately in the output component.
static constexpr ToshibaFamilyCapabilityEntry TOSHIBA_FAMILY_CAPABILITIES[] = {
    {ToshibaIndoorUnitFamily::J2FVG,
     COMMON_RESIDENTIAL_FEATURES |
         FEATURE_FLOOR |
         FEATURE_AIR_OUTLET_SELECT},
    {ToshibaIndoorUnitFamily::G3KVSG,
     COMMON_RESIDENTIAL_FEATURES |
         FEATURE_HORIZONTAL_AIRFLOW |
         FEATURE_HADA_CARE},
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

// Mode matrix below is intentionally evidence-limited. It records the Function
// and Fan controls observed in the genuine Toshiba app on the directly tested
// RAS-B10P2KVSGB-E. Do not copy these rules to another family until validated.
static constexpr ToshibaModeFunctionEntry TOSHIBA_VALIDATED_MODE_FUNCTIONS[] = {
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
  if (model.find("G3KVSG") != std::string::npos) return ToshibaIndoorUnitFamily::G3KVSG;
  if (model.find("P2KVSG") != std::string::npos) return ToshibaIndoorUnitFamily::P2KVSG;
  return ToshibaIndoorUnitFamily::UNKNOWN;
}

const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family) {
  switch (family) {
    case ToshibaIndoorUnitFamily::J2FVG: return "J2FVG";
    case ToshibaIndoorUnitFamily::G3KVSG: return "G3KVSG";
    case ToshibaIndoorUnitFamily::P2KVSG: return "P2KVSG";
    default: return "UNKNOWN";
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

ToshibaCapabilityProfile power_select_cancel_profile(ToshibaIndoorUnitFamily family,
                                                      ToshibaHvacMode mode) {
  ToshibaCapabilityProfile profile;

  // Directly observed in the P2 app: changing Power Select in Auto/Cool/Heat
  // cancels ECO, Hi POWER and Silent Operation back to Standard. PURE remains
  // independent and is deliberately not included in this mask.
  if (family == ToshibaIndoorUnitFamily::P2KVSG &&
      (mode == ToshibaHvacMode::AUTO || mode == ToshibaHvacMode::COOL ||
       mode == ToshibaHvacMode::HEAT)) {
    profile.features = FEATURE_ECO | FEATURE_HI_POWER | FEATURE_OUTDOOR_SILENT;
  }

  return profile;
}

ToshibaEquipmentIdentification decode_equipment_identification(const std::vector<uint8_t> &raw_data) {
  ToshibaEquipmentIdentification result;

  // Captured equipment-identification publication:
  //   02 00 03 11 .. .. 6A 01 30 01 00 65 E0 [100-byte payload] checksum
  // Payload = two 50-byte equipment records.
  if (raw_data.size() < 114 || raw_data[0] != 0x02 || raw_data[2] != 0x03 || raw_data[3] != 0x11 ||
      raw_data[12] != 0xE0) {
    return result;
  }

  constexpr size_t IDU_RECORD_OFFSET = 13;
  constexpr size_t ODU_RECORD_OFFSET = IDU_RECORD_OFFSET + 50;
  constexpr size_t MODEL_FIELD_WIDTH = 21;
  constexpr size_t IDENTITY_1_OFFSET = 21;
  constexpr size_t IDENTITY_1_WIDTH = 13;
  constexpr size_t IDENTITY_2_OFFSET = 34;
  constexpr size_t IDENTITY_2_WIDTH = 9;
  constexpr size_t IDENTITY_3_OFFSET = 43;
  constexpr size_t IDENTITY_3_WIDTH = 7;

  result.valid = true;

  result.idu_model = decode_ascii_field(raw_data, IDU_RECORD_OFFSET, MODEL_FIELD_WIDTH);
  result.idu_identity_1 = decode_ascii_field(raw_data, IDU_RECORD_OFFSET + IDENTITY_1_OFFSET, IDENTITY_1_WIDTH);
  result.idu_identity_2 = decode_ascii_field(raw_data, IDU_RECORD_OFFSET + IDENTITY_2_OFFSET, IDENTITY_2_WIDTH);
  result.idu_identity_3 = decode_ascii_field(raw_data, IDU_RECORD_OFFSET + IDENTITY_3_OFFSET, IDENTITY_3_WIDTH);

  result.odu_model = decode_ascii_field(raw_data, ODU_RECORD_OFFSET, MODEL_FIELD_WIDTH);
  result.odu_identity_1 = decode_ascii_field(raw_data, ODU_RECORD_OFFSET + IDENTITY_1_OFFSET, IDENTITY_1_WIDTH);
  result.odu_identity_2 = decode_ascii_field(raw_data, ODU_RECORD_OFFSET + IDENTITY_2_OFFSET, IDENTITY_2_WIDTH);
  result.odu_identity_3 = decode_ascii_field(raw_data, ODU_RECORD_OFFSET + IDENTITY_3_OFFSET, IDENTITY_3_WIDTH);

  result.idu_model_available = is_model_field_available(result.idu_model);
  result.odu_model_available = is_model_field_available(result.odu_model);

  if (!is_identity_field_available(result.idu_identity_1)) result.idu_identity_1.clear();
  if (!is_identity_field_available(result.idu_identity_2)) result.idu_identity_2.clear();
  if (!is_identity_field_available(result.idu_identity_3)) result.idu_identity_3.clear();
  if (!is_identity_field_available(result.odu_identity_1)) result.odu_identity_1.clear();
  if (!is_identity_field_available(result.odu_identity_2)) result.odu_identity_2.clear();
  if (!is_identity_field_available(result.odu_identity_3)) result.odu_identity_3.clear();

  if (result.idu_model_available) {
    result.idu_family = indoor_unit_family_from_model(result.idu_model);
    result.capabilities = capability_profile_from_model(result.idu_model);
  } else {
    result.idu_model.clear();
    result.capabilities.features = FEATURE_COMMON_HVAC;
  }

  if (!result.odu_model_available) result.odu_model.clear();

  return result;
}

}  // namespace toshiba_suzumi
}  // namespace esphome
