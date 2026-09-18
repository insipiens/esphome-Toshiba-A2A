#include "toshiba_device_profile.h"

#include "toshiba_family_j2fvg.h"
#include "toshiba_family_p2kvsg.h"

namespace esphome {
namespace toshiba_a2a {

const ToshibaFamilyProfile &profile_for_model(const std::string &model) {
  if (model.find("J2FVG") != std::string::npos) return j2fvg::PROFILE;
  if (model.find("P2KVSG") != std::string::npos) return p2kvsg::PROFILE;
  return UNKNOWN_FAMILY_PROFILE;
}

const ToshibaFamilyProfile &profile_for_family(ToshibaIndoorUnitFamily family) {
  switch (family) {
    case ToshibaIndoorUnitFamily::J2FVG:
      return j2fvg::PROFILE;
    case ToshibaIndoorUnitFamily::P2KVSG:
      return p2kvsg::PROFILE;
    default:
      return UNKNOWN_FAMILY_PROFILE;
  }
}

const char *indoor_unit_family_to_string(ToshibaIndoorUnitFamily family) {
  switch (family) {
    case ToshibaIndoorUnitFamily::J2FVG:
      return "J2FVG";
    case ToshibaIndoorUnitFamily::P2KVSG:
      return "P2KVSG";
    default:
      return "UNKNOWN";
  }
}

}  // namespace toshiba_a2a
}  // namespace esphome
