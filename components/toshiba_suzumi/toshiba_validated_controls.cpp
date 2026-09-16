#include "toshiba_climate.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_suzumi {

namespace {

ToshibaHvacMode climate_to_toshiba_mode(climate::ClimateMode mode) {
  switch (mode) {
    case climate::CLIMATE_MODE_HEAT_COOL: return ToshibaHvacMode::AUTO;
    case climate::CLIMATE_MODE_COOL: return ToshibaHvacMode::COOL;
    case climate::CLIMATE_MODE_HEAT: return ToshibaHvacMode::HEAT;
    case climate::CLIMATE_MODE_DRY: return ToshibaHvacMode::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY: return ToshibaHvacMode::FAN;
    default: return ToshibaHvacMode::UNKNOWN;
  }
}

ToshibaFeature feature_for_special_mode(SPECIAL_MODE mode) {
  switch (mode) {
    case SPECIAL_MODE::ECO: return FEATURE_ECO;
    case SPECIAL_MODE::HI_POWER: return FEATURE_HI_POWER;
    case SPECIAL_MODE::SILENT_1:
    case SPECIAL_MODE::SILENT_2: return FEATURE_OUTDOOR_SILENT;
    case SPECIAL_MODE::EIGHT_DEG: return FEATURE_EIGHT_DEG_HEAT;
    case SPECIAL_MODE::SLEEP: return FEATURE_SLEEP;
    case SPECIAL_MODE::FLOOR: return FEATURE_FLOOR;
    case SPECIAL_MODE::COMFORT: return FEATURE_COMFORT;
    case SPECIAL_MODE::FIREPLACE_1:
    case SPECIAL_MODE::FIREPLACE_2: return FEATURE_FIREPLACE;
    default: return FEATURE_NONE;
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

SPECIAL_MODE current_special_mode(const optional<SPECIAL_MODE> &mode) {
  return mode.has_value() ? mode.value() : SPECIAL_MODE::STANDARD;
}

bool has_validated_mode_profile(ToshibaIndoorUnitFamily family) {
  return family == ToshibaIndoorUnitFamily::J2FVG || family == ToshibaIndoorUnitFamily::P2KVSG;
}

bool climate_call_is_swing_only(const climate::ClimateCall &call) {
  return call.get_swing_mode().has_value() && !call.get_mode().has_value() &&
         !call.get_target_temperature().has_value() && !call.get_fan_mode().has_value() &&
         !call.has_custom_fan_mode() && !call.get_preset().has_value() && !call.has_custom_preset();
}

}  // namespace

void ToshibaValidatedControlUart::setup() {
  ToshibaDiagnosticMonitorUart::setup();
  this->set_supported_custom_fan_modes({
      CUSTOM_FAN_AUTO,
      CUSTOM_FAN_QUIET,
      CUSTOM_FAN_LOW,
      CUSTOM_FAN_LEVEL_2,
      CUSTOM_FAN_MEDIUM,
      CUSTOM_FAN_LEVEL_4,
      CUSTOM_FAN_HIGH,
  });
}

climate::ClimateTraits ToshibaValidatedControlUart::traits() {
  auto traits = ToshibaDiagnosticMonitorUart::traits();
  traits.set_supported_fan_modes({});
  return traits;
}

ToshibaHvacMode ToshibaValidatedControlUart::current_hvac_mode_() const {
  return climate_to_toshiba_mode(this->mode);
}

bool ToshibaValidatedControlUart::validated_function_allowed_(ToshibaFeature feature, ToshibaHvacMode mode) const {
  if (!has_validated_mode_profile(this->idu_family_) || mode == ToshibaHvacMode::UNKNOWN) return true;
  return validated_function_profile_for_mode(this->idu_family_, mode).has(feature);
}

bool ToshibaValidatedControlUart::validated_fan_allowed_(uint8_t fan_option, ToshibaHvacMode mode) const {
  if (!has_validated_mode_profile(this->idu_family_) || mode == ToshibaHvacMode::UNKNOWN) return true;
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
    ESP_LOGW(TAG, "%s is not available in the current HVAC mode", SpecialModeToPreset(mode));
    this->publish_validated_f7_mode_(current_special_mode(this->special_mode_));
    return;
  }

  if (!enabled && (!this->special_mode_.has_value() || this->special_mode_.value() != mode)) {
    this->publish_validated_f7_mode_(current_special_mode(this->special_mode_));
    return;
  }

  const SPECIAL_MODE requested = enabled ? mode : SPECIAL_MODE::STANDARD;
  this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->requestData(ToshibaCommandType::SPECIAL_MODE);
}

void ToshibaValidatedControlUart::on_set_validated_silent_(const std::string &value) {
  SPECIAL_MODE requested = SPECIAL_MODE::STANDARD;
  if (value == "Silent 1") requested = SPECIAL_MODE::SILENT_1;
  else if (value == "Silent 2") requested = SPECIAL_MODE::SILENT_2;
  else if (value != "Standard") {
    ESP_LOGW(TAG, "Unknown Silent Operation option: %s", value.c_str());
    return;
  }

  if (requested != SPECIAL_MODE::STANDARD &&
      !this->validated_function_allowed_(FEATURE_OUTDOOR_SILENT, this->current_hvac_mode_())) {
    ESP_LOGW(TAG, "Silent Operation is not available in the current HVAC mode");
    this->publish_validated_f7_mode_(current_special_mode(this->special_mode_));
    return;
  }

  this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->requestData(ToshibaCommandType::SPECIAL_MODE);
}

void ToshibaValidatedControlUart::on_set_validated_power_level_(const std::string &value) {
  auto pwr_level = StringToPwrLevel(value);
  if (!pwr_level.has_value()) {
    ESP_LOGW(TAG, "Unknown Power Select value: %s", value.c_str());
    return;
  }

  if (this->special_mode_.has_value()) {
    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());
    const auto cancel = power_select_cancel_profile(this->idu_family_, this->current_hvac_mode_());
    if (active_feature != FEATURE_NONE && cancel.has(active_feature)) {
      this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));
      this->requestData(ToshibaCommandType::SPECIAL_MODE);
    }
  }

  this->sendCmd(ToshibaCommandType::POWER_SEL, static_cast<uint8_t>(pwr_level.value()));
  this->requestData(ToshibaCommandType::POWER_SEL);
}

void ToshibaValidatedControlUart::on_set_pure_(bool enabled) {
  if (!this->validated_function_allowed_(FEATURE_PURE, this->current_hvac_mode_())) {
    ESP_LOGW(TAG, "PURE is not available in the current HVAC mode");
    if (this->pure_switch_ != nullptr) this->pure_switch_->publish_state(!enabled);
    return;
  }
  this->sendCmd(ToshibaCommandType::PURE, enabled ? 0x18 : 0x10);
  this->requestData(ToshibaCommandType::PURE);
}

void ToshibaValidatedControlUart::on_press_defrost_(bool strong) {
  const ToshibaHvacMode mode = this->current_hvac_mode_();
  if (strong) {
    if (this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG && mode != ToshibaHvacMode::HEAT) {
      ESP_LOGW(TAG, "Strong Defrost is only exposed in Heat on the validated P2 path");
      return;
    }
    this->sendCmd(ToshibaCommandType::DEFROST, 0x01);
    return;
  }

  if (!this->validated_function_allowed_(FEATURE_START_DEFROST, mode)) {
    ESP_LOGW(TAG, "Start Defrost is not available in the current HVAC mode");
    return;
  }
  this->sendCmd(ToshibaCommandType::DEFROST, 0x02);
}

void ToshibaValidatedControlUart::on_set_vertical_fixed_position_(const std::string &value) {
  auto index = FixedPositionIndexFromName(value);
  if (!index.has_value()) {
    ESP_LOGW(TAG, "Unknown vertical FIX index: %s", value.c_str());
    return;
  }

  const uint8_t requested_vertical = index.value();

  if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG) {
    const uint8_t raw = static_cast<uint8_t>(0x4F + requested_vertical);  // 50..54
    ESP_LOGD(TAG, "Requesting J2 vertical FIX %s -> A3=%02X", value.c_str(), raw);
    this->sendCmd(ToshibaCommandType::SWING, raw);
    this->requestData(ToshibaCommandType::SWING);
    return;
  }

  if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG) {
    ESP_LOGW(TAG, "Vertical FIX requested with unknown/unsupported IDU family");
    return;
  }

  const uint8_t raw = EncodePackedFixPosition(this->fix_horizontal_index_, requested_vertical);
  ESP_LOGD(TAG, "Requesting P2 vertical FIX %s -> A3=%02X (retained H=%u, requested V=%u)%s", value.c_str(), raw,
           this->fix_horizontal_index_, requested_vertical,
           this->have_packed_fix_state_ ? "" : " using provisional retained H");
  this->sendCmd(ToshibaCommandType::SWING, raw);
  this->requestData(ToshibaCommandType::SWING);
}

void ToshibaValidatedControlUart::on_set_horizontal_air_direction_(const std::string &value) {
  if (this->idu_family_ != ToshibaIndoorUnitFamily::P2KVSG) {
    ESP_LOGW(TAG, "Horizontal FIX is only implemented for the P2 family");
    return;
  }

  auto index = FixedPositionIndexFromName(value);
  if (!index.has_value()) {
    ESP_LOGW(TAG, "Unknown horizontal FIX index: %s", value.c_str());
    return;
  }

  const uint8_t requested_horizontal = index.value();
  const uint8_t raw = EncodePackedFixPosition(requested_horizontal, this->fix_vertical_index_);
  ESP_LOGD(TAG, "Requesting P2 horizontal FIX %s -> A3=%02X (requested H=%u, retained V=%u)%s", value.c_str(), raw,
           requested_horizontal, this->fix_vertical_index_,
           this->have_packed_fix_state_ ? "" : " using provisional retained V");
  this->sendCmd(ToshibaCommandType::SWING, raw);
  this->requestData(ToshibaCommandType::SWING);
}

void ToshibaValidatedControlUart::publish_packed_fix_state_(uint8_t raw) {
  uint8_t horizontal = 0;
  uint8_t vertical = 0;
  if (!DecodePackedFixPosition(raw, horizontal, vertical)) return;

  this->fix_horizontal_index_ = horizontal;
  this->fix_vertical_index_ = vertical;
  this->have_packed_fix_state_ = true;

  const char *horizontal_name = HorizontalFixedPositionName(horizontal);
  if (horizontal_name != nullptr && this->horizontal_air_direction_select_ != nullptr)
    this->horizontal_air_direction_select_->publish_state(horizontal_name);

  const char *vertical_name = VerticalFixedPositionName(vertical);
  if (vertical_name != nullptr && this->vertical_air_direction_select_ != nullptr)
    this->vertical_air_direction_select_->publish_state(vertical_name);

  this->swing_mode = climate::CLIMATE_SWING_OFF;
  this->publish_state();

  ESP_LOGD(TAG, "A3 packed FIX state %02X -> H=%u V=%u", raw, horizontal, vertical);
}

void ToshibaValidatedControlUart::publish_horizontal_air_direction_(uint8_t raw) {
  this->publish_packed_fix_state_(raw);
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
      ESP_LOGW(TAG, "Requested fan mode is not available in the current HVAC mode");
      return;
    }
  }

  if (call.has_custom_fan_mode()) {
    const auto requested_fan = StringToFanLevel(call.get_custom_fan_mode().c_str());
    if (!requested_fan.has_value()) {
      ESP_LOGW(TAG, "Unknown Toshiba fan mode: %s", call.get_custom_fan_mode().c_str());
      return;
    }
    uint8_t option = FAN_OPTION_MANUAL;
    if (requested_fan.value() == FAN::FAN_AUTO) option = FAN_OPTION_AUTO;
    else if (requested_fan.value() == FAN::FAN_QUIET) option = FAN_OPTION_QUIET;
    if (!this->validated_fan_allowed_(option, requested_mode)) {
      ESP_LOGW(TAG, "Requested fan mode is not available in the current HVAC mode");
      return;
    }
  }

  if (call.get_target_temperature().has_value() && has_validated_mode_profile(this->idu_family_) &&
      *call.get_target_temperature() < MIN_TEMP_STANDARD && requested_mode != ToshibaHvacMode::HEAT) {
    ESP_LOGW(TAG, "8 °C heat setpoints are only valid in Heat on the validated J2/P2 path");
    return;
  }

  if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG && call.get_swing_mode().has_value()) {
    const auto requested_swing = *call.get_swing_mode();
    uint8_t raw = 0x31;
    if (requested_swing == climate::CLIMATE_SWING_VERTICAL) raw = 0x41;
    else if (requested_swing != climate::CLIMATE_SWING_OFF) {
      ESP_LOGW(TAG, "J2 supports vertical swing only");
      return;
    }

    ESP_LOGD(TAG, "Requesting J2 swing %s -> A3=%02X", climate_swing_mode_to_string(requested_swing), raw);
    this->sendCmd(ToshibaCommandType::SWING, raw);
    this->requestData(ToshibaCommandType::SWING);

    if (climate_call_is_swing_only(call)) return;

    // Do not pass a combined J2 swing call into the inherited P2-oriented A3
    // encoder. HA normally issues swing as a separate climate call.
    ESP_LOGW(TAG, "Combined J2 swing + climate-property call rejected; issue the swing change separately");
    return;
  }

  if (call.get_mode().has_value() && *call.get_mode() != climate::CLIMATE_MODE_OFF && this->special_mode_.has_value()) {
    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());
    if (active_feature != FEATURE_NONE && !this->validated_function_allowed_(active_feature, requested_mode)) {
      this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));
      this->requestData(ToshibaCommandType::SPECIAL_MODE);
    }
  }

  ToshibaClimateUart::control(call);

  if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_DRY &&
      has_validated_mode_profile(this->idu_family_)) {
    this->sendCmd(ToshibaCommandType::FAN, static_cast<uint8_t>(FAN::FAN_AUTO));
    this->requestData(ToshibaCommandType::FAN);
  }
}

void ToshibaValidatedControlUart::parseResponse(std::vector<uint8_t> raw) {
  const int16_t response_register = this->extract_response_register_(raw);
  uint8_t value = 0;

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::FAN) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::FAN), value)) {
    const char *fan_mode = IntToCustomFanMode(static_cast<FAN>(value));
    if (std::strcmp(fan_mode, "Unknown") != 0) {
      ESP_LOGI(TAG, "Received Toshiba fan mode: %s", fan_mode);
      this->set_custom_fan_mode_(fan_mode);
      this->publish_state();
      return;
    }
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::PURE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::PURE), value)) {
    if (this->register_c7_raw_sensor_ != nullptr) this->register_c7_raw_sensor_->publish_state(value);
    if (this->pure_switch_ != nullptr) {
      if (value == 0x18) this->pure_switch_->publish_state(true);
      else if (value == 0x10) this->pure_switch_->publish_state(false);
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::MAINTENANCE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::MAINTENANCE), value)) {
    const auto state = static_cast<MAINTENANCE_STATE>(value);
    switch (state) {
      case MAINTENANCE_STATE::IDLE:
        this->set_self_clean_running_(false);
        if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(false);
        ESP_LOGI(TAG, "CB maintenance state: idle");
        break;
      case MAINTENANCE_STATE::STRONG_DEFROST:
        this->set_self_clean_running_(false);
        if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(true);
        ESP_LOGI(TAG, "CB maintenance state: Strong Defrost active");
        break;
      case MAINTENANCE_STATE::NORMAL_DEFROST:
        this->set_self_clean_running_(false);
        if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(true);
        ESP_LOGI(TAG, "CB maintenance state: Normal Defrost active");
        break;
      case MAINTENANCE_STATE::SELF_CLEAN:
        if (this->defrost_active_sensor_ != nullptr) this->defrost_active_sensor_->publish_state(false);
        this->set_self_clean_running_(true);
        ESP_LOGI(TAG, "CB maintenance state: IDU Self Clean active");
        break;
      default:
        ESP_LOGW(TAG, "Unknown CB maintenance state: 0x%02X", value);
        break;
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::SWING) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaCommandType::SWING), value)) {
    if (this->idu_family_ == ToshibaIndoorUnitFamily::J2FVG && value >= 0x50 && value <= 0x54) {
      const uint8_t vertical = static_cast<uint8_t>(value - 0x4F);
      const char *vertical_name = VerticalFixedPositionName(vertical);
      if (vertical_name != nullptr && this->vertical_air_direction_select_ != nullptr)
        this->vertical_air_direction_select_->publish_state(vertical_name);
      this->swing_mode = climate::CLIMATE_SWING_OFF;
      this->publish_state();
      ESP_LOGD(TAG, "J2 A3 FIX state %02X -> V=%u", value, vertical);
      return;
    }

    if (this->idu_family_ == ToshibaIndoorUnitFamily::P2KVSG)
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

void ToshibaValidatedVerticalAirDirectionSelect::control(const std::string &value) {
  this->parent_->on_set_vertical_fixed_position_(value);
}

void ToshibaHorizontalAirDirectionSelect::control(const std::string &value) {
  this->parent_->on_set_horizontal_air_direction_(value);
}

}  // namespace toshiba_suzumi
}  // namespace esphome