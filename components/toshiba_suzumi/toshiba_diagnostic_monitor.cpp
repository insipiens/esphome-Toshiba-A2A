#include <algorithm>
#include <utility>
#include "toshiba_climate.h"
#include "toshiba_model.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_suzumi {

static constexpr size_t CHUNK = 24;

void ToshibaClimateUart::set_detected_equipment_(const ToshibaEquipmentIdentification &equipment) {
  // The protocol layer only accepts positive identity data from Toshiba.
  // Blank/NULL E0 fields are ignored so they cannot erase a previously known
  // runtime identity. Persistence across reboots belongs in the ESPHome YAML.
  if (equipment.idu_model_available && !equipment.idu_model.empty()) {
    if (this->idu_model_ != equipment.idu_model) {
      this->idu_model_ = equipment.idu_model;
      this->idu_family_ = equipment.idu_family;
      this->capabilities_ = equipment.capabilities;
      ESP_LOGI(TAG, "E0 IDU model: %s", this->idu_model_.c_str());
      ESP_LOGI(TAG, "E0 IDU family: %s", indoor_unit_family_to_string(this->idu_family_));
      if (this->idu_model_sensor_ != nullptr) this->idu_model_sensor_->publish_state(this->idu_model_);
    }
  } else {
    ESP_LOGD(TAG, "E0 IDU model unavailable; retaining current runtime identity");
  }

  if (!equipment.idu_identity_1.empty() && this->idu_identity_1_ != equipment.idu_identity_1) {
    this->idu_identity_1_ = equipment.idu_identity_1;
    ESP_LOGI(TAG, "E0 IDU identity 1: %s", this->idu_identity_1_.c_str());
    if (this->idu_identity_1_sensor_ != nullptr) this->idu_identity_1_sensor_->publish_state(this->idu_identity_1_);
  }
  if (!equipment.idu_identity_2.empty() && this->idu_identity_2_ != equipment.idu_identity_2) {
    this->idu_identity_2_ = equipment.idu_identity_2;
    ESP_LOGI(TAG, "E0 IDU identity 2: %s", this->idu_identity_2_.c_str());
    if (this->idu_identity_2_sensor_ != nullptr) this->idu_identity_2_sensor_->publish_state(this->idu_identity_2_);
  }
  if (!equipment.idu_identity_3.empty() && this->idu_identity_3_ != equipment.idu_identity_3) {
    this->idu_identity_3_ = equipment.idu_identity_3;
    ESP_LOGI(TAG, "E0 IDU identity 3: %s", this->idu_identity_3_.c_str());
    if (this->idu_identity_3_sensor_ != nullptr) this->idu_identity_3_sensor_->publish_state(this->idu_identity_3_);
  }

  if (equipment.odu_model_available && !equipment.odu_model.empty()) {
    if (this->odu_model_ != equipment.odu_model) {
      this->odu_model_ = equipment.odu_model;
      ESP_LOGI(TAG, "E0 ODU model: %s", this->odu_model_.c_str());
      if (this->odu_model_sensor_ != nullptr) this->odu_model_sensor_->publish_state(this->odu_model_);
    }
  } else {
    ESP_LOGD(TAG, "E0 ODU model unavailable; retaining current runtime identity");
  }

  if (!equipment.odu_identity_1.empty() && this->odu_identity_1_ != equipment.odu_identity_1) {
    this->odu_identity_1_ = equipment.odu_identity_1;
    ESP_LOGI(TAG, "E0 ODU identity 1: %s", this->odu_identity_1_.c_str());
    if (this->odu_identity_1_sensor_ != nullptr) this->odu_identity_1_sensor_->publish_state(this->odu_identity_1_);
  }
  if (!equipment.odu_identity_2.empty() && this->odu_identity_2_ != equipment.odu_identity_2) {
    this->odu_identity_2_ = equipment.odu_identity_2;
    ESP_LOGI(TAG, "E0 ODU identity 2: %s", this->odu_identity_2_.c_str());
    if (this->odu_identity_2_sensor_ != nullptr) this->odu_identity_2_sensor_->publish_state(this->odu_identity_2_);
  }
  if (!equipment.odu_identity_3.empty() && this->odu_identity_3_ != equipment.odu_identity_3) {
    this->odu_identity_3_ = equipment.odu_identity_3;
    ESP_LOGI(TAG, "E0 ODU identity 3: %s", this->odu_identity_3_.c_str());
    if (this->odu_identity_3_sensor_ != nullptr) this->odu_identity_3_sensor_->publish_state(this->odu_identity_3_);
  }
}

void ToshibaClimateUart::publish_special_mode_entities_(SPECIAL_MODE mode) {
  if (mode == SPECIAL_MODE::STANDARD) {
    if (this->eco_switch_ != nullptr) this->eco_switch_->publish_state(false);
    if (this->hi_power_switch_ != nullptr) this->hi_power_switch_->publish_state(false);
    if (this->eight_degree_heat_switch_ != nullptr) this->eight_degree_heat_switch_->publish_state(false);
    if (this->sleep_switch_ != nullptr) this->sleep_switch_->publish_state(false);
    if (this->floor_switch_ != nullptr) this->floor_switch_->publish_state(false);
    if (this->comfort_switch_ != nullptr) this->comfort_switch_->publish_state(false);
    if (this->fireplace_select_ != nullptr) this->fireplace_select_->publish_state("Off");
    if (this->outdoor_silent_select_ != nullptr) this->outdoor_silent_select_->publish_state("Off");
    return;
  }

  switch (mode) {
    case SPECIAL_MODE::HI_POWER:
      if (this->hi_power_switch_ != nullptr) this->hi_power_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::ECO:
      if (this->eco_switch_ != nullptr) this->eco_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::EIGHT_DEG:
      if (this->eight_degree_heat_switch_ != nullptr) this->eight_degree_heat_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::SLEEP:
      if (this->sleep_switch_ != nullptr) this->sleep_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::FLOOR:
      if (this->floor_switch_ != nullptr) this->floor_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::COMFORT:
      if (this->comfort_switch_ != nullptr) this->comfort_switch_->publish_state(true);
      break;
    case SPECIAL_MODE::FIREPLACE_1:
      if (this->fireplace_select_ != nullptr) this->fireplace_select_->publish_state("Fireplace 1");
      break;
    case SPECIAL_MODE::FIREPLACE_2:
      if (this->fireplace_select_ != nullptr) this->fireplace_select_->publish_state("Fireplace 2");
      break;
    case SPECIAL_MODE::SILENT_1:
      if (this->outdoor_silent_select_ != nullptr) this->outdoor_silent_select_->publish_state("Silent 1");
      break;
    case SPECIAL_MODE::SILENT_2:
      if (this->outdoor_silent_select_ != nullptr) this->outdoor_silent_select_->publish_state("Silent 2");
      break;
    default:
      break;
  }
}

void ToshibaClimateUart::on_set_special_mode_switch(SPECIAL_MODE mode, bool enabled) {
  ESP_LOGD(TAG, "Setting divided Toshiba function %s to %s", SpecialModeToPreset(mode), enabled ? "ON" : "OFF");
  this->sendCmd(ToshibaCommandType::SPECIAL_MODE,
                static_cast<uint8_t>(enabled ? mode : SPECIAL_MODE::STANDARD));
}

void ToshibaClimateUart::on_set_special_mode_level(SPECIAL_MODE level_one, SPECIAL_MODE level_two,
                                                    const std::string &option_one,
                                                    const std::string &option_two,
                                                    const std::string &value) {
  SPECIAL_MODE mode = SPECIAL_MODE::STANDARD;
  if (value == option_one) {
    mode = level_one;
  } else if (value == option_two) {
    mode = level_two;
  } else if (value != "Off") {
    ESP_LOGW(TAG, "Unknown divided Toshiba level option: %s", value.c_str());
    return;
  }
  ESP_LOGD(TAG, "Setting divided Toshiba level function to %s", value.c_str());
  this->sendCmd(ToshibaCommandType::SPECIAL_MODE, static_cast<uint8_t>(mode));
}

void ToshibaSpecialModeSwitch::write_state(bool state) {
  this->parent_->on_set_special_mode_switch(this->mode_, state);
  this->publish_state(state);
}

void ToshibaSpecialModeLevelSelect::control(const std::string &value) {
  this->parent_->on_set_special_mode_level(this->level_one_, this->level_two_,
                                           this->option_one_, this->option_two_, value);
}

void ToshibaDiagnosticMonitorUart::update() {
  // The development monitor is passive. Normal polling must continue while the
  // switch is enabled, so temporarily hide the monitor flag from the base
  // update() guard.
  const bool monitoring = this->scan_active_;
  if (monitoring) this->scan_active_ = false;
  ToshibaClimateUart::update();
  if (monitoring) this->scan_active_ = true;
}

void ToshibaDiagnosticMonitorUart::parseResponse(std::vector<uint8_t> raw) {
  const int16_t response_register = this->extract_response_register_(raw);

  if (this->scan_active_) {
    ESP_LOGI(TAG, "UART MONITOR RX class=0x%02X reg=%s length=%u checksum=OK",
             raw.size() > 3 ? static_cast<unsigned>(raw[3]) : 0U,
             response_register >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(response_register)).c_str() : "unknown",
             static_cast<unsigned>(raw.size()));
    this->log_monitor_bytes_(raw, response_register);
    this->log_monitor_decoded_(raw, response_register);
  }

  if (raw.size() > 12 && raw[3] == 0x11 &&
      raw[12] == static_cast<uint8_t>(ToshibaCommandType::EQUIPMENT_INFO)) {
    const auto equipment = decode_equipment_identification(raw);
    if (!equipment.valid) {
      ESP_LOGW(TAG, "E0 equipment-identification packet did not match the expected class-0x11 layout");
      return;
    }
    this->set_detected_equipment_(equipment);
    return;
  }

  // Suppress the temporary verbose E4/E5 base dumps while retaining sensors.
  if (response_register == static_cast<uint8_t>(ToshibaCommandType::ODU_STATUS) &&
      (raw.size() == 22 || raw.size() == 24)) {
    const uint8_t offset = (raw.size() == 22) ? 13 : 15;
    if (this->odu_discharge_temp_sensor_ != nullptr) {
      const int8_t val = static_cast<int8_t>(raw[offset + 0]);
      if (val != 127) this->odu_discharge_temp_sensor_->publish_state(val);
    }
    if (this->odu_suction_temp_sensor_ != nullptr) {
      const int8_t val = static_cast<int8_t>(raw[offset + 1]);
      if (val != 127) this->odu_suction_temp_sensor_->publish_state(val);
    }
    if (this->odu_heat_exchanger_temp_sensor_ != nullptr) {
      const int8_t val = static_cast<int8_t>(raw[offset + 2]);
      if (val != 127) this->odu_heat_exchanger_temp_sensor_->publish_state(val);
    }
    if (this->compressor_load_sensor_ != nullptr) {
      const uint8_t raw_val = raw[offset + 3];
      if (raw_val < 254) this->compressor_load_sensor_->publish_state(raw_val / 1.7f);
    }
    if (this->compressor_current_sensor_ != nullptr) {
      const uint8_t raw_val = raw[offset + 6];
      if (raw_val < 254) this->compressor_current_sensor_->publish_state(raw_val / 10.0f * 0.827f);
    }
    return;
  }

  if (response_register == static_cast<uint8_t>(ToshibaCommandType::IDU_STATUS) &&
      (raw.size() == 22 || raw.size() == 24)) {
    const uint8_t offset = (raw.size() == 22) ? 13 : 15;
    if (this->idu_heat_exchanger_temp_sensor_ != nullptr) {
      const int8_t val = static_cast<int8_t>(raw[offset + 0]);
      if (val != 127) this->idu_heat_exchanger_temp_sensor_->publish_state(val);
    }
    if (this->idu_junction_temp_sensor_ != nullptr) {
      const int8_t val = static_cast<int8_t>(raw[offset + 1]);
      if (val != 127) this->idu_junction_temp_sensor_->publish_state(val);
    }
    if (this->idu_fan_speed_sensor_ != nullptr) {
      this->idu_fan_speed_sensor_->publish_state(raw[offset + 2]);
    }
    return;
  }

  SPECIAL_MODE received_mode;
  bool have_special_mode = false;
  if (raw.size() == 15 && raw[12] == static_cast<uint8_t>(ToshibaCommandType::SPECIAL_MODE)) {
    received_mode = static_cast<SPECIAL_MODE>(raw[13]);
    have_special_mode = true;
  } else if (raw.size() == 17 && raw[14] == static_cast<uint8_t>(ToshibaCommandType::SPECIAL_MODE)) {
    received_mode = static_cast<SPECIAL_MODE>(raw[15]);
    have_special_mode = true;
  }
  if (have_special_mode) this->publish_special_mode_entities_(received_mode);

  ToshibaClimateUart::parseResponse(std::move(raw));
}

void ToshibaDiagnosticMonitorUart::set_scan_enabled(bool enabled) {
  if (enabled == this->scan_active_) return;

  this->scan_active_ = enabled;
  this->scan_started_ = false;
  this->scan_request_sent_ = false;
  this->scan_matched_response_ = false;
  this->monitor_stop_requested_ = false;

  if (enabled) {
    this->monitor_cycle_started_ = millis();
    this->monitor_last_payload_.fill({});
    this->monitor_payload_seen_.fill(false);
    ESP_LOGI(TAG, "========== TOSHIBA PASSIVE UART MONITOR STARTED ==========");
    ESP_LOGI(TAG, "capturing every received Toshiba frame; no diagnostic register polling is generated");
    ESP_LOGI(TAG, "normal command TX remains visible through the existing ToshibaCommand debug log");
  } else {
    ESP_LOGI(TAG, "========== TOSHIBA PASSIVE UART MONITOR STOPPED ==========");
  }
}

void ToshibaDiagnosticMonitorUart::process_scan_() {
  // Intentionally empty: the development monitor is passive and must not alter
  // bus traffic or pause ordinary climate communication.
}

void ToshibaDiagnosticMonitorUart::send_monitor_request_() {}
void ToshibaDiagnosticMonitorUart::complete_monitor_request_() {}

void ToshibaDiagnosticMonitorUart::finish_monitor_() {
  this->set_scan_enabled(false);
}

bool ToshibaDiagnosticMonitorUart::extract_monitor_payload_(const std::vector<uint8_t> &raw,
                                                             int16_t reg,
                                                             std::vector<uint8_t> &payload) const {
  if (reg < 0) return false;

  size_t offset;
  if ((raw.size() == 15 || raw.size() == 22) && raw.size() > 12 && raw[12] == reg) {
    offset = 12;
  } else if (raw.size() > 14 && raw[3] == 0x90 && raw[14] == reg) {
    offset = 14;
  } else if (raw.size() > 12 && raw[12] == reg) {
    offset = 12;
  } else {
    return false;
  }

  if (offset + 1 > raw.size() - 1) return false;
  payload.assign(raw.begin() + offset + 1, raw.end() - 1);
  return true;
}

void ToshibaDiagnosticMonitorUart::remember_monitor_payload_(uint8_t reg,
                                                              const std::vector<uint8_t> &payload) {
  if (reg < 0x80) return;
  const size_t index = reg - 0x80;
  if (index < this->monitor_last_payload_.size()) {
    this->monitor_last_payload_[index] = payload;
    this->monitor_payload_seen_[index] = true;
  }
}

void ToshibaDiagnosticMonitorUart::log_timer_bank_snapshot_() const {
  // Retained for compatibility with older development builds. Passive monitor
  // mode no longer generates a synthetic register-bank snapshot.
}

void ToshibaDiagnosticMonitorUart::log_scan_packet_(const std::vector<uint8_t> &raw) {
  // No active scan requests are issued in passive-monitor mode, but if an older
  // call path reaches here, treat the packet exactly like ordinary live traffic.
  this->parseResponse(raw);
}

void ToshibaDiagnosticMonitorUart::log_monitor_bytes_(const std::vector<uint8_t> &raw, int16_t reg) const {
  const size_t chunk_count = (raw.size() + CHUNK - 1) / CHUNK;
  for (size_t chunk = 0; chunk < chunk_count; chunk++) {
    const size_t offset = chunk * CHUNK;
    const size_t size = std::min(CHUNK, raw.size() - offset);

    if (reg >= 0) {
      ESP_LOGI(TAG, "UART RAW RX reg=0x%02X chunk=%u/%u offset=%u bytes=[%s]",
               static_cast<unsigned>(reg), static_cast<unsigned>(chunk + 1),
               static_cast<unsigned>(chunk_count), static_cast<unsigned>(offset),
               format_hex_pretty(raw.data() + offset, size).c_str());
    } else {
      ESP_LOGI(TAG, "UART RAW RX reg=unknown chunk=%u/%u offset=%u bytes=[%s]",
               static_cast<unsigned>(chunk + 1), static_cast<unsigned>(chunk_count),
               static_cast<unsigned>(offset), format_hex_pretty(raw.data() + offset, size).c_str());
    }
  }
}

void ToshibaDiagnosticMonitorUart::log_monitor_decoded_(const std::vector<uint8_t> &raw, int16_t reg) {
  std::vector<uint8_t> payload;
  if (this->extract_monitor_payload_(raw, reg, payload)) {
    this->remember_monitor_payload_(static_cast<uint8_t>(reg), payload);
    ESP_LOGI(TAG, "UART MONITOR VALUE reg=0x%02X value=[%s] len=%u",
             static_cast<unsigned>(reg), format_hex_pretty(payload).c_str(),
             static_cast<unsigned>(payload.size()));
  }
}

}  // namespace toshiba_suzumi
}  // namespace esphome