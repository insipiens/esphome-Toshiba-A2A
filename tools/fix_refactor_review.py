from pathlib import Path

root = Path('.')
comp = root / 'components' / 'toshiba_a2a'

# Clean device-profile public interface: declarative capability/encoding data only.
(comp / 'toshiba_device_profile.h').write_text(r'''#pragma once

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
''')

(comp / 'toshiba_identity.h').write_text(r'''#pragma once

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

ToshibaEquipmentIdentification decode_equipment_identification(const std::vector<uint8_t> &raw_data);

}  // namespace toshiba_a2a
}  // namespace esphome
''')

# Remove obsolete Power Select cancellation helper from profile implementation.
p = comp / 'toshiba_device_profile.cpp'
s = p.read_text()
start = s.find('ToshibaCapabilityProfile power_select_cancel_profile(')
if start != -1:
    end = s.find('\n\n}  // namespace toshiba_a2a', start)
    if end == -1:
        raise RuntimeError('Could not locate end of power_select_cancel_profile')
    s = s[:start] + s[end:]
p.write_text(s)

# Fixed-position entities are normal ESPHome entities. Installation profile controls
# exposure; E0 reports state/identity and does not control entity registration.
p = comp / 'climate.py'
s = p.read_text()
helper_start = s.find('async def _new_deferred_select(')
helper_end = s.find('\nasync def to_code(config):', helper_start)
if helper_start != -1 and helper_end != -1:
    s = s[:helper_start] + s[helper_end + 1:]
s = s.replace(
'''    if CONF_VERTICAL_AIR_DIRECTION in config:\n        sel = await _new_deferred_select(config[CONF_VERTICAL_AIR_DIRECTION], config, vertical_fixed_options)\n        cg.add(var.set_deferred_vertical_air_direction_select(sel))\n    if CONF_HORIZONTAL_AIR_DIRECTION in config:\n        sel = await _new_deferred_select(config[CONF_HORIZONTAL_AIR_DIRECTION], config, horizontal_fixed_options)\n        cg.add(var.set_deferred_horizontal_air_direction_select(sel))\n''',
'''    if CONF_VERTICAL_AIR_DIRECTION in config:\n        sel = await select.new_select(config[CONF_VERTICAL_AIR_DIRECTION], options=vertical_fixed_options)\n        await cg.register_parented(sel, config[CONF_ID])\n        cg.add(var.set_vertical_air_direction_select(sel))\n    if CONF_HORIZONTAL_AIR_DIRECTION in config:\n        sel = await select.new_select(config[CONF_HORIZONTAL_AIR_DIRECTION], options=horizontal_fixed_options)\n        await cg.register_parented(sel, config[CONF_ID])\n        cg.add(var.set_horizontal_air_direction_select(sel))\n''')
p.write_text(s)

# Remove E0-driven optional entity registration and its state from C++.
p = comp / 'toshiba_climate.h'
s = p.read_text()
s = s.replace('    this->on_reported_idu_model_available_();\n', '')
s = s.replace('  virtual void on_reported_idu_model_available_() {}\n', '')
s = s.replace('  void set_deferred_vertical_air_direction_select(ToshibaValidatedVerticalAirDirectionSelect *sel) {\n    deferred_vertical_air_direction_select_ = sel;\n  }\n  void set_deferred_horizontal_air_direction_select(ToshibaHorizontalAirDirectionSelect *sel) {\n    deferred_horizontal_air_direction_select_ = sel;\n  }\n', '')
s = s.replace('  void on_reported_idu_model_available_() override;\n', '')
s = s.replace('  ToshibaValidatedVerticalAirDirectionSelect *deferred_vertical_air_direction_select_ = nullptr;\n  ToshibaHorizontalAirDirectionSelect *deferred_horizontal_air_direction_select_ = nullptr;\n  bool vertical_air_direction_registered_{false};\n  bool horizontal_air_direction_registered_{false};\n', '')
p.write_text(s)

p = comp / 'toshiba_validated_controls.cpp'
s = p.read_text()
s = s.replace('#include "esphome/core/application.h"\n', '')
start = s.find('void ToshibaValidatedControlUart::on_reported_idu_model_available_() {')
if start != -1:
    end = s.find('\n}\n\nclimate::ClimateTraits ToshibaValidatedControlUart::traits()', start)
    if end == -1:
        raise RuntimeError('Could not locate end of on_reported_idu_model_available_')
    s = s[:start] + s[end + 3:]
s = s.replace('''  if (this->special_mode_.has_value()) {\n    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());\n    const auto cancel = power_select_cancel_profile(this->idu_family_, this->current_hvac_mode_());\n    if (active_feature != FEATURE_NONE && cancel.has(active_feature)) {\n      this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));\n      this->requestData(ToshibaCommandType::SPECIAL_MODE);\n    }\n  }\n\n''', '')
s = s.replace('''  if (this->get_reported_idu_model().empty()) {\n    ESP_LOGW(TAG, "Vertical FIX unavailable: no usable IDU model reported by E0");\n    return;\n  }\n\n''', '')
s = s.replace('''        if (!this->get_reported_idu_model().empty() && vertical_name != nullptr &&\n            this->vertical_air_direction_select_ != nullptr)\n''', '''        if (vertical_name != nullptr && this->vertical_air_direction_select_ != nullptr)\n''')
p.write_text(s)

# E0 should update identity/profile state, not register entities.
p = comp / 'toshiba_diagnostic_monitor.cpp'
s = p.read_text().replace('    this->on_reported_idu_model_available_();\n', '')
p.write_text(s)

# Record the significant cleanup.
p = root / 'CHANGELOG.txt'
s = p.read_text()
marker = 'Sep 16th 2026\n\n'
entry = ('* refactor branch review cleanup: repaired generated profile/identity headers, removed E0-driven FIX registration/write gates, made FIX selects normal ESPHome entities governed by the explicit installation capability profile, and removed the obsolete Power Select cancellation path so functional code follows the current interaction policy\n')
if entry not in s:
    s = s.replace(marker, marker + entry, 1)
p.write_text(s)
