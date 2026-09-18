#pragma once

#include <string>

#include "toshiba_family.h"

namespace esphome {
namespace toshiba_a2a {

const ToshibaFamilyProfile &profile_for_model(const std::string &model);
const ToshibaFamilyProfile &profile_for_family(ToshibaIndoorUnitFamily family);
const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family);

}  // namespace toshiba_a2a
}  // namespace esphome
