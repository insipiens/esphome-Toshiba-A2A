#include "toshiba_identity.h"
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace toshiba_a2a {

struct ToshibaEquipmentIdentification {
  bool valid{false};
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
};

// Decode the pushed class-0x11 / register-0xE0 equipment-identification payload.
ToshibaEquipmentIdentification decode_equipment_identification(const std::vector<uint8_t> &raw_data);

}  // namespace toshiba_a2a
}  // namespace esphome
