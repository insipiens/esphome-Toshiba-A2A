#include "toshiba_identity.h"

#include <cctype>

namespace esphome {
namespace toshiba_a2a {

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


}  // namespace

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


}  // namespace toshiba_a2a
}  // namespace esphome
