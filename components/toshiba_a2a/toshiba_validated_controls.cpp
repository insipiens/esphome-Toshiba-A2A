#include "toshiba_climate.h"
#include "toshiba_family_j2fvg.h"
#include "toshiba_family_p2kvsg.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_a2a {

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

bool climate_call_is_swing_only(const climate::ClimateCall &call) {
  return call.get_swing_mode().has_value() && !call.get_mode().has_value() &&
         !call.get_target_temperature().has_value() && !call.get_fan_mode().has_value() &&
         !call.has_custom_fan_mode() && !call.get_preset().has_value() && !call.has_custom_preset();
}

}  // namespace

void ToshibaValidatedControlUart::setup() {
  ToshibaDiagnosticMonitorUart::setup();
  // ESPHome already has native Auto / Quiet / Low / Medium / High fan modes.
  // Only Toshiba's two intermediate levels need to be exposed as custom modes.
  this->set_supported_custom_fan_modes({
      CUSTOM_FAN_LEVEL_2,
      CUSTOM_FAN_LEVEL_4,
  });
}


climate::ClimateTraits ToshibaValidatedControlUart::traits() {
  // Keep the base component's native Auto / Quiet / Low / Medium / High modes.
  // The two Toshiba-only intermediate fan levels are registered as custom modes
  // during setup().
  return ToshibaDiagnosticMonitorUart::traits();
}

ToshibaHvacMode ToshibaValidatedControlUart::current_hvac_mode_() const {
  return climate_to_toshiba_mode(this->mode);
}

bool ToshibaValidatedControlUart::validated_function_allowed_(ToshibaFeature feature, ToshibaHvacMode mode) const {
  if (!has_validated_mode_profile(*this->family_profile_) || mode == ToshibaHvacMode::UNKNOWN) return true;
  return validated_function_profile_for_mode(*this->family_profile_, mode).has(feature);
}

bool ToshibaValidatedControlUart::validated_fan_allowed_(uint8_t fan_option, ToshibaHvacMode mode) const {
  if (!has_validated_mode_profile(*this->family_profile_) || mode == ToshibaHvacMode::UNKNOWN) return true;
  const uint8_t options = validated_fan_options_for_mode(*this->family_profile_, mode);
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
  this->sendCmd(ToshibaRegister::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->requestData(ToshibaRegister::SPECIAL_MODE);
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

  this->sendCmd(ToshibaRegister::SPECIAL_MODE, static_cast<uint8_t>(requested));
  this->requestData(ToshibaRegister::SPECIAL_MODE);
}

void ToshibaValidatedControlUart::on_set_validated_power_level_(const std::string &value) {
  auto pwr_level = StringToPwrLevel(value);
  if (!pwr_level.has_value()) {
    ESP_LOGW(TAG, "Unknown Power Select value: %s", value.c_str());
    return;
  }

  this->sendCmd(ToshibaRegister::POWER_SELECT, static_cast<uint8_t>(pwr_level.value()));
  this->requestData(ToshibaRegister::POWER_SELECT);
}

void ToshibaValidatedControlUart::on_set_pure_(bool enabled) {
  if (!this->validated_function_allowed_(FEATURE_PURE, this->current_hvac_mode_())) {
    ESP_LOGW(TAG, "PURE is not available in the current HVAC mode");
    if (this->pure_switch_ != nullptr) this->pure_switch_->publish_state(!enabled);
    return;
  }
  this->sendCmd(ToshibaRegister::PURE, enabled ? static_cast<uint8_t>(reg_c7::PureState::ON) : static_cast<uint8_t>(reg_c7::PureState::OFF));
  this->requestData(ToshibaRegister::PURE);
}

void ToshibaValidatedControlUart::on_press_defrost_(bool strong) {
  const ToshibaHvacMode mode = this->current_hvac_mode_();
  if (strong) {
    if (!validated_strong_defrost_allowed(*this->family_profile_, mode)) {
      ESP_LOGW(TAG, "Strong Defrost is only exposed in Heat on the validated P2 path");
      return;
    }
    this->sendCmd(ToshibaRegister::MAINTENANCE, static_cast<uint8_t>(reg_cb::Command::STRONG_DEFROST));
    return;
  }

  if (!this->validated_function_allowed_(FEATURE_START_DEFROST, mode)) {
    ESP_LOGW(TAG, "Start Defrost is not available in the current HVAC mode");
    return;
  }
  this->sendCmd(ToshibaRegister::MAINTENANCE, static_cast<uint8_t>(reg_cb::Command::NORMAL_DEFROST));
}

void ToshibaValidatedControlUart::on_set_vertical_fixed_position_(const std::string &value) {
  auto index = FixedPositionIndexFromName(value);
  if (!index.has_value()) {
    ESP_LOGW(TAG, "Unknown vertical FIX index: %s", value.c_str());
    return;
  }

  const uint8_t requested_vertical = index.value();

  if (this->family_profile_->louvre_encoding == ToshibaLouvreEncoding::J2_VERTICAL) {
    const uint8_t raw = j2fvg::a3::encode_fixed(requested_vertical);  // 50..54
    ESP_LOGD(TAG, "Requesting J2 vertical FIX %s -> A3=%02X", value.c_str(), raw);
    this->sendCmd(ToshibaRegister::LOUVRE, raw);
    this->requestData(ToshibaRegister::LOUVRE);
    return;
  }

  if (this->family_profile_->louvre_encoding != ToshibaLouvreEncoding::P2_PACKED) {
    ESP_LOGW(TAG, "Vertical FIX requested with unknown/unsupported louvre encoding");
    return;
  }

  const uint8_t raw = p2kvsg::a3::encode_packed_fix(this->fix_horizontal_index_, requested_vertical);
  ESP_LOGD(TAG, "Requesting P2 vertical FIX %s -> A3=%02X (retained H=%u, requested V=%u)%s", value.c_str(), raw,
           this->fix_horizontal_index_, requested_vertical,
           this->have_packed_fix_state_ ? "" : " using provisional retained H");
  this->sendCmd(ToshibaRegister::LOUVRE, raw);
  this->requestData(ToshibaRegister::LOUVRE);
}

void ToshibaValidatedControlUart::on_set_horizontal_air_direction_(const std::string &value) {
  if (this->family_profile_->louvre_encoding != ToshibaLouvreEncoding::P2_PACKED) {
    ESP_LOGW(TAG, "Horizontal FIX is unavailable for this louvre encoding");
    return;
  }

  auto index = FixedPositionIndexFromName(value);
  if (!index.has_value()) {
    ESP_LOGW(TAG, "Unknown horizontal FIX index: %s", value.c_str());
    return;
  }

  const uint8_t requested_horizontal = index.value();
  const uint8_t raw = p2kvsg::a3::encode_packed_fix(requested_horizontal, this->fix_vertical_index_);
  ESP_LOGD(TAG, "Requesting P2 horizontal FIX %s -> A3=%02X (requested H=%u, retained V=%u)%s", value.c_str(), raw,
           requested_horizontal, this->fix_vertical_index_,
           this->have_packed_fix_state_ ? "" : " using provisional retained V");
  this->sendCmd(ToshibaRegister::LOUVRE, raw);
  this->requestData(ToshibaRegister::LOUVRE);
}

void ToshibaValidatedControlUart::publish_packed_fix_state_(uint8_t raw) {
  uint8_t horizontal = 0;
  uint8_t vertical = 0;
  if (!p2kvsg::a3::decode_packed_fix(raw, horizontal, vertical)) return;

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

  if (call.get_target_temperature().has_value() && has_validated_mode_profile(*this->family_profile_) &&
      *call.get_target_temperature() < MIN_TEMP_STANDARD && requested_mode != ToshibaHvacMode::HEAT) {
    ESP_LOGW(TAG, "8 °C heat setpoints are only valid in Heat on the validated J2/P2 path");
    return;
  }

  if (this->family_profile_->louvre_encoding == ToshibaLouvreEncoding::J2_VERTICAL &&
      call.get_swing_mode().has_value()) {
    const auto requested_swing = *call.get_swing_mode();
    uint8_t raw = j2fvg::a3::OFF;
    if (requested_swing == climate::CLIMATE_SWING_VERTICAL) raw = j2fvg::a3::VERTICAL_SWING;
    else if (requested_swing != climate::CLIMATE_SWING_OFF) {
      ESP_LOGW(TAG, "J2 supports vertical swing only");
      return;
    }

    ESP_LOGD(TAG, "Requesting J2 swing %s -> A3=%02X", climate_swing_mode_to_string(requested_swing), raw);
    this->sendCmd(ToshibaRegister::LOUVRE, raw);
    this->requestData(ToshibaRegister::LOUVRE);

    if (climate_call_is_swing_only(call)) return;

    // Do not pass a combined J2 swing call into the inherited P2-oriented A3
    // encoder. HA normally issues swing as a separate climate call.
    ESP_LOGW(TAG, "Combined J2 swing + climate-property call rejected; issue the swing change separately");
    return;
  }

  if (call.get_mode().has_value() && *call.get_mode() != climate::CLIMATE_MODE_OFF && this->special_mode_.has_value()) {
    const ToshibaFeature active_feature = feature_for_special_mode(this->special_mode_.value());
    if (active_feature != FEATURE_NONE && !this->validated_function_allowed_(active_feature, requested_mode)) {
      this->sendCmd(ToshibaRegister::SPECIAL_MODE, static_cast<uint8_t>(SPECIAL_MODE::STANDARD));
      this->requestData(ToshibaRegister::SPECIAL_MODE);
    }
  }

  ToshibaClimateUart::control(call);

  if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_DRY &&
      has_validated_mode_profile(*this->family_profile_)) {
    this->sendCmd(ToshibaRegister::FAN, static_cast<uint8_t>(FAN::FAN_AUTO));
    this->requestData(ToshibaRegister::FAN);
  }
}

void ToshibaValidatedControlUart::parseResponse(std::vector<uint8_t> raw) {
  const int16_t response_register = this->extract_response_register_(raw);
  uint8_t value = 0;

  // FAN readback deliberately falls through to ToshibaClimateUart via the
  // diagnostic layer. The base parser maps Auto / Quiet / Low / Medium / High
  // to native ESPHome fan modes and only Toshiba levels 2 and 4 to custom fan
  // modes, matching the traits exposed to Home Assistant.

  if (response_register == static_cast<uint8_t>(ToshibaRegister::PURE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaRegister::PURE), value)) {
    if (this->register_c7_raw_sensor_ != nullptr) this->register_c7_raw_sensor_->publish_state(value);
    if (this->pure_switch_ != nullptr) {
      if (value == static_cast<uint8_t>(reg_c7::PureState::ON)) this->pure_switch_->publish_state(true);
      else if (value == static_cast<uint8_t>(reg_c7::PureState::OFF)) this->pure_switch_->publish_state(false);
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaRegister::MAINTENANCE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaRegister::MAINTENANCE), value)) {
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

  if (response_register == static_cast<uint8_t>(ToshibaRegister::LOUVRE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaRegister::LOUVRE), value)) {
    if (this->family_profile_->louvre_encoding == ToshibaLouvreEncoding::J2_VERTICAL) {
      uint8_t vertical = 0;
      if (j2fvg::a3::decode_fixed(value, vertical)) {
        const char *vertical_name = VerticalFixedPositionName(vertical);
        if (vertical_name != nullptr && this->vertical_air_direction_select_ != nullptr)
          this->vertical_air_direction_select_->publish_state(vertical_name);
        this->swing_mode = climate::CLIMATE_SWING_OFF;
        this->publish_state();
        ESP_LOGD(TAG, "J2 A3 FIX state %02X -> V=%u", value, vertical);
        return;
      }

      if (value == j2fvg::a3::OFF) {
        this->swing_mode = climate::CLIMATE_SWING_OFF;
        this->publish_state();
        ESP_LOGI(TAG, "Received J2 swing mode: OFF");
        return;
      }
      if (value == j2fvg::a3::VERTICAL_SWING) {
        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
        this->publish_state();
        ESP_LOGI(TAG, "Received J2 swing mode: VERTICAL");
        return;
      }

      ESP_LOGD(TAG, "Ignoring unrecognised J2 A3 state 0x%02X for FIX selector", value);
      return;
    }

    if (this->family_profile_->louvre_encoding == ToshibaLouvreEncoding::P2_PACKED)
      this->publish_horizontal_air_direction_(value);
  }

  if (response_register == static_cast<uint8_t>(ToshibaRegister::SPECIAL_MODE) &&
      extract_scalar(raw, static_cast<uint8_t>(ToshibaRegister::SPECIAL_MODE), value)) {
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
void ToshibaValidatedSpecialModeLevelSelect::control(const std::string &value) {
  SPECIAL_MODE mode=SPECIAL_MODE::STANDARD;
  if(value==this->option_one_) mode=this->level_one_; else if(value==this->option_two_) mode=this->level_two_;
  else if(value!="Off"){ESP_LOGW(TAG,"Unknown Toshiba level option: %s",value.c_str());return;}
  this->parent_->on_set_validated_special_mode_(mode,mode!=SPECIAL_MODE::STANDARD);
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

}  // namespace toshiba_a2a
}  // namespace esphome
