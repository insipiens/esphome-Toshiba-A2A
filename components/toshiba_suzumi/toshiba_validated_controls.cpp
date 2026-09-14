#include "toshiba_climate.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_suzumi {

namespace {

ToshibaHvacMode climate_to_toshiba_mode(climate::ClimateMode mode) {
  switch (mode) {
    case climate::CLIMATE_MODE_HEAT_COOL:
      return ToshibaHvacMode::AUTO;
    case climate::CLIMATE_MODE_COOL:
      return ToshibaHvacMode::COOL;
    case climate::CLIMATE_MODE_HEAT:
      return ToshibaHvacMode::HEAT;
    case climate::CLIMATE_MODE_DRY:
      return ToshibaHvacMode::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return ToshibaHvacMode::FAN;
    default:
      return ToshibaHvacMode::UNKNOWN;
  }
}

ToshibaFeature feature_for_special_mode(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::ECO:
      return FEATURE_ECO;
    case SPECIAL_MODE::HI_POWER:
      return FEATURE_HI_POWER;
    case SPECIAL_MODE::SILENT_1:
    case SPECIAL_MODE::SILENT_2:
      return FEATURE_OUTDOOR_SILENT;
    case SPECIAL_MODE::EIGHT_DEG:
      return FEATURE_EIGHT_DEG_HEAT;
    case SPECIAL_MODE::SLEEP:
      return FEATURE_SLEEP;
    case SPECIAL_MODE::FLOOR:
      return FEATURE_FLOOR;
    case SPECIAL_MODE::COMFORT:
      return FEATURE_COMFORT;
    case SPECIAL_MODE::FIREPLACE_1:
    case SPECIAL_MODE::FIREPLACE_2:
      return FEATURE_FIREPLACE;
    default:
      return FEATURE_NONE;
  }
}

bool extract_scalar(const std::vector<uint8_t> &raw, uint8_t reg, uint8_t &value) {
  if (raw.size() == 15 && raw[12] == reg) {
    value = raw[13];
    return true;
  }
  if (raw.size() == 17 && raw[14] == reg) {
    value = raw[15];
    return true;
  }
  return false;
}

optional<uint8_t> fixed_position_index(const std::string &value) {
  for (uint8_t i = 0; i < 6; i++) {
    const char *name = FixedPositionName(i);
    if (name != nullptr && str_equals_case_insensitive(value, name)) return i;
  }
  return nullopt;
}

}  // namespace

ToshibaHvacMode ToshibaValidatedControlUart::current_hvac_mode_() const {
  return climate_to_toshiba_mode(this->mode);
}

bool ToshibaValidatedControlUart::validated_function_allowed_(ToshibaFeature feature, ToshibaHvacMode mode) const {
  // The mode matrix is directly validated only on P2KVSG. Do not impose P2
  // restrictions on families for which we do not yet have equivalent tests.
  if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG || mode == ToshibaHvacMode::UNKNOWN) return true;
  return validated_function_profile_for_mode(this->idu_family_, mode).has(feature);
}

bool ToshibaValidatedControlUart::validated_fan_allowed_(uint8_t fan_option, ToshibaHvacMode mode) const {
  if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG || mode == ToshibaHvacMode::UNKNOWN) return true;
  const uint8_t options = validated_fan_options_for_mode(this->idu_family_, mode);
  return (options & fan_option) != 0;
}

void ToshibaValidatedControlUart::clear_validated_f7_entities_() {
  if (this->validated_eco_switch_ != nullptr) this->validated_eco_switch_->publish_state(false);
  if (this->validated_hi_power_switch_ != nullptr) this->validated_hi_power_switch_->publish_state(false);
  if (this->validated_eight_degree_heat_switch_ != nullptr) this->validated_eight_degree_heat_switch_->publish_state(false);
  if (this->validated_outdoor_silent_select_ != nullptr) this->validated_outdoor_silent_select_->publish_state("Standard");
}

void ToshibaValidatedControlUart::publish_validated_f7_mode_(SPECIAL_MODE mode) {
  // F7 itself is a selector. Its divided Home Assistant entities therefore
  // remain mutually exclusive. Independent Function controls such as PURE and
  // Power Select are deliberately not touched here.
  this->clear_validated_f7_entities_();
  switch (mode) {
    case SPECIAL_MODE::ECO:
      if (this->validated_eco_switch_ != nullptr) this->validated_eco_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::HI_POWER:
      if (this->validated_hi_power_switch_ != nullptr) this->validated_hi_power_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::EIGHT_DEG:
      if (this->validated_eight_degree_heat_switch_ != nullptr)
        this->validated_eight_degree_heat_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::SILENT_1:
      if (this->validated_outdoor_silent_select_ != nullptr)
        this->validated_outdoor_silent_select_->publish_state("Silent 1");
      break;
    case SPECIAL_MODE::SILENT_2:
      if (this->validated_outdoor_silent_select_ != nullptr)
        this->validated_outdoor_silent_select_->publish_state("Silent 2");
      break;
    default:
      break;
  }
}

void ToshibaValidatedControlUart::on_set_validated_special_mode_(SPECIAL_MODE mode, bool enabled) {
  const ToshibaFeature feature = feature_for_special_mode(mode);
  const ToshibaHvacMode hvac_mode = this->current_hvac_mode_();
  if (enabled && feature != FEATURE_NONE && !this->validated_function_allowed_(feature, hvac_mode)) {
    ESP_LOGW(TAG, "%s is not available in the current P2 HVAC mode", SpecialModeToPreset(mode));
    this->publish_validated_f7_mode_(this->special_mode_.value_or(SPECIAL_MODE::STANDARD));
    return;
  }

  if (!enabled && (!this->special_mode_.has_value() || this->special_mode_.value() != mode)) {
    this->publish_validated_f7_mode_(this->special_mode_.value_or(SPECIAL_MODE::STANDARD));
    return;
  }

  const SPECIAL_MODE requested = enabled ? mode : SPECIAL_MODE::STANDARD;
  ESP_LOGD(TAG, "Setting validated Toshiba Function %s", SpecialModeToPreset(requested));
  this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->special_mode_ = requested;
  this->publish_validated_f7_mode_(requested);
}

void ToshibaValidatedControlUart::on_set_validated_silent_(const std::string &value) {
  SPECIAL_MODE requested = SPECIAL_MODE::STANDARD;
  if (value == "Silent 1") {
    requested = SPECIAL_MODE::SILENT_1;
  } else if (value == "Silent 2") {
    requested = SPECIAL_MODE::SILENT_2;
  } else if (value != "Standard") {
    ESP_LOGW(TAG, "Unknown Silent Operation option: %s", value.c_str());
    return;
  }

  if (requested != SPECIAL_MODE::STANDARD &&
      !this->validated_function_allowed_(FEATURE_OUTDOOR_SILENT, this->current_hvac_mode_())) {
    ESP_LOGW(TAG, "Silent Operation is not available in the current P2 HVAC mode");
    this->publish_validated_f7_mode_(this->special_mode_.value_or(SPECIAL_MODE::STANDARD));
    return;
  }

  this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->special_mode_ = requested;
  this->publish_validated_f7_mode_(requested);
}

void ToshibaValidatedControlUart::on_set_validated_power_level_(const std::string &value) {
  auto pwr_level = StringToPwrLevel(value);
  if (!pwr_level.has_value()) {
    ESP_LOGW(TAG, "Unknown Power Select value: %s", value.c_str());
    return;
  }

  // Direct P2 capture: changing Power Select in Auto/Cool/Heat cancels the
  // F7 performance functions ECO, Hi POWER and Silent Operation. PURE is on
  // C7 and remains independent.
  if (this->special_mode_.has_value()) {
    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());
    const auto cancel = power_select_cancel_profile(this->idu_family_, this->current_hvac_mode_());
    if (active_feature != FEATURE_NONE && cancel.has(active_feature)) {
      this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));
      this->special_mode_ = SPECIAL_MODE::STANDARD;
      this->publish_validated_f7_mode_(SPECIAL_MODE::STANDARD);
    }
  }

  ESP_LOGD(TAG, "Setting Power Select to %s", value.c_str());
  this->sendCmd(ToshibaCommandType::POWER_SEL, static_cast<uint8_t>(pwr_level.value()));
  if (this->pwr_select_ != nullptr) this->pwr_select_->publish_state(value);
}

void ToshibaValidatedControlUart::on_set_pure_(bool enabled) {
  if (!this->validated_function_allowed_(FEATURE_PURE, this->current_hvac_mode_())) {
    ESP_LOGW(TAG, "PURE is not available in the current P2 HVAC mode");
    if (this->pure_switch_ != nullptr) this->pure_switch_->publish_state(!enabled);
    return;
  }
  this->sendCmd(ToshibaCommandType::PURE, enabled ? 0x18 : 0x10);
  if (this->pure_switch_ != nullptr) this->pure_switch_->publish_state(enabled);
}

void ToshibaValidatedControlUart::on_press_defrost_(bool strong) {
  const ToshibaHvacMode mode = this->current_hvac_mode_();
  if (strong) {
    // Strong Defrost has been directly captured in Heat. Do not generalise it
    // to Auto until the same operation is observed there.
    if (this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG && mode != ToshibaHvacMode::HEAT) {
      ESP_LOGW(TAG, "Strong Defrost is only exposed in Heat on the validated P2 path");
      return;
    }
    this->sendCmd(ToshibaCommandType::DEFROST, 0x01);
    return;
  }

  if (!this->validated_function_allowed_(FEATURE_START_DEFROST, mode)) {
    ESP_LOGW(TAG, "Start Defrost is not available in the current P2 HVAC mode");
    return;
  }
  this->sendCmd(ToshibaCommandType::DEFROST, 0x02);
}

void ToshibaValidatedControlUart::on_set_horizontal_air_direction_(const std::string &value) {
  uint8_t raw = 0;
  if (str_equals_case_insensitive(value, "Off")) {
    raw = static_cast<uint8_t>(SWING::OFF);
  } else if (str_equals_case_insensitive(value, "Swing")) {
    raw = static_cast<uint8_t>(SWING::HORIZONTAL);
  } else {
    auto index = fixed_position_index(value);
    if (!index.has_value()) {
      ESP_LOGW(TAG, "Unknown horizontal air direction: %s", value.c_str());
      return;
    }
    // Genuine adaptor ordered horizontal FIX sweep:
    // 85 8D 95 9D A5 AD (six positions, +8 per step).
    raw = static_cast<uint8_t>(0x85 + (index.value() * 0x08));
  }

  ESP_LOGD(TAG, "Setting horizontal air direction to %s (A3=%02X)", value.c_str(), raw);
  this->sendCmd(ToshibaCommandType::SWING, raw);
  if (this->horizontal_air_direction_select_ != nullptr)
    this->horizontal_air_direction_select_->publish_state(value);

  if (raw == static_cast<uint8_t>(SWING::HORIZONTAL)) {
    this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
  } else {
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }
  this->publish_state();
}

void ToshibaValidatedControlUart::publish_horizontal_air_direction_(uint8_t raw) {
  if (this->horizontal_air_direction_select_ == nullptr) return;

  if (raw == static_cast<uint8_t>(SWING::HORIZONTAL) || raw == static_cast<uint8_t>(SWING::BOTH)) {
    this->horizontal_air_direction_select_->publish_state("Swing");
    return;
  }
  if (raw == static_cast<uint8_t>(SWING::OFF) || raw == static_cast<uint8_t>(SWING::VERTICAL)) {
    this->horizontal_air_direction_select_->publish_state("Off");
    return;
  }

  uint8_t horizontal_index = 0;
  uint8_t vertical_index = 0;
  if (DecodePackedFixPosition(raw, horizontal_index, vertical_index)) {
    const char *name = FixedPositionName(horizontal_index);
    if (name != nullptr) this->horizontal_air_direction_select_->publish_state(name);
  }
}

void ToshibaValidatedControlUart::control(const climate::ClimateCall &call) {
  ToshibaHvacMode requested_mode = this->current_hvac_mode_();
  if (call.get_mode().has_value() && *call.get_mode() != climate::CLIMATE_MODE_OFF)
    requested_mode = climate_to_toshiba_mode(*call.get_mode());

  if (call.get_fan_mode().has_value()) {
    uint8_t option = FAN_OPTION_MANUAL;
    if (*call.get_fan_mode() == climate::CLIMATE_FAN_AUTO) option = FAN_OPTION_AUTO;
    if (*call.get_fan_mode() == climate::CLIMATE_FAN_QUIET) option = FAN_OPTION_QUIET;
    if (!this->validated_fan_allowed_(option, requested_mode)) {
      ESP_LOGW(TAG, "Requested fan mode is not available in the current P2 HVAC mode");
      return;
    }
  }
  if (call.has_custom_fan_mode() && !this->validated_fan_allowed_(FAN_OPTION_MANUAL, requested_mode)) {
    ESP_LOGW(TAG, "Requested manual fan level is not available in the current P2 HVAC mode");
    return;
  }

  if (call.get_target_temperature().has_value() && this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG &&
      *call.get_target_temperature() < MIN_TEMP_STANDARD && requested_mode != ToshibaHvacMode::HEAT) {
    ESP_LOGW(TAG, "8 °C heat setpoints are only valid in Heat on the P2 unit");
    return;
  }

  // If a mode change makes the active F7 function invalid, cancel the F7
  // selector before applying the new operating mode. Independent Function
  // controls (PURE/Power Select) remain untouched.
  if (call.get_mode().has_value() && *call.get_mode() != climate::CLIMATE_MODE_OFF && this->special_mode_.has_value()) {
    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());
    if (active_feature != FEATURE_NONE && !this->validated_function_allowed_(active_feature, requested_mode)) {
      this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));
      this->special_mode_ = SPECIAL_MODE::STANDARD;
      this->publish_validated_f7_mode_(SPECIAL_MODE::STANDARD);
    }
  }

  ToshibaDiagnosticMonitorUart::control(call);

  // Toshiba app behaviour on the directly tested P2: Dry exposes Auto only.
  if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_DRY &&
      this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG) {
    this->sendCmd(ToshibaCommandType::FAN, static_cast<uint8_t>(FAN::FAN_AUTO));
    this->set_fan_mode_(climate::CLIMATE_FAN_AUTO);
    this->publish_state();
  }
}

void ToshibaValidatedControlUart::parseResponse(std::vector<uint8_t> raw) {
  const int16_t response_register = this->extract_response_register_(raw);
  uint8_t value = 0;

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::PURE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::PURE), value)) {
    if (this->register_c7_raw_sensor_ != nullptr) this->register_c7_raw_sensor_->publish_state(value);
    if (this->pure_switch_ != nullptr) {
      if (value == 0x18) this->pure_switch_->publish_state(true);
      else if (value == 0x10) this->pure_switch_->publish_state(false);
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::DEFROST) &&
      this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::DEFROST), value)) {
    if (value == 0x10) {
      if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(false);
    } else if (value == 0x11) {
      if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(true);
    } else if (value == 0x12) {
      // 0x12 is still only the expected Normal Defrost active partner. Preserve
      // it as an observation without promoting the inferred mapping to state.
      ESP_LOGI(TAG, "Observed CB 12; Normal Defrost active mapping still awaits explicit confirmation");
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::SWING) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::SWING), value)) {
    this->publish_horizontal_air_direction_(value);
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::SPECIAL_MODE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::SPECIAL_MODE), value)) {
    const auto mode = static_cast<SPECIAL_MODE>(value);
    this->special_mode_ = mode;
    this->publish_validated_f7_mode_(mode);
  }

  ToshibaDiagnosticMonitorUart::parseResponse(std::move(raw));
}

void ToshibaValidatedFunctionSwitch::write_state(bool state) {
  this->parent_->on_set_validated_special_mode_(this->mode_, state);
}

void ToshibaValidatedSilentSelect::control(const std::string &value) {
  this->parent_->on_set_validated_silent_(value);
}

void ToshibaValidatedPowerSelect::control(const std::string &value) {
  this->parent_->on_set_validated_power_level_(value);
}

void ToshibaPureSwitch::write_state(bool state) {
  this->parent_->on_set_pure_(state);
}

void ToshibaDefrostButton::press_action() {
  this->parent_->on_press_defrost_(this->strong_);
}

void ToshibaHorizontalAirDirectionSelect::control(const std::string &value) {
  this->parent_->on_set_horizontal_air_direction_(value);
}

}  // namespace toshiba_suzumi
}  // namespace esphome
