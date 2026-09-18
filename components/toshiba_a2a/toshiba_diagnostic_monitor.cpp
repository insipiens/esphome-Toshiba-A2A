#include <algorithm>
#include <utility>
#include "toshiba_climate.h"
#include "toshiba_device_profile.h"
#include "toshiba_identity.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_a2a {

void ToshibaClimateUart::set_detected_equipment_(const ToshibaEquipmentIdentification &equipment) {
  // The protocol layer only accepts positive identity data from Toshiba.
  // Blank/NULL E0 fields are ignored so they cannot erase a previously known
  // runtime identity. Persistence across reboots belongs in the ESPHome YAML.
  if (equipment.idu_model_available && !equipment.idu_model.empty()) {
    if (this->idu_model_ != equipment.idu_model) {
      this->idu_model_ = equipment.idu_model;
      this->family_profile_ = &profile_for_model(equipment.idu_model);
      ESP_LOGI(TAG, "E0 IDU model: %s", this->idu_model_.c_str());
      ESP_LOGI(TAG, "E0 IDU family: %s", indoor_unit_family_to_string(this->family_profile_->family));
      if (this->idu_model_sensor_ != nullptr) this->idu_model_sensor_->publish_state(this->idu_model_);
    }
    // Optional model-dependent entities are deliberately registered only after
    // Toshiba itself reports a usable IDU model in E0. A YAML model override
    // still selects protocol routing, but it does not bypass this UI gate.
  } else {
    ESP_LOGD(TAG, "E0 IDU model unavailable; retaining current runtime identity and conservative UI");
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

void ToshibaDiagnosticMonitorUart::update() {
  // Normal component polling must continue while the passive focused monitor
  // is enabled. Temporarily hide the monitor flag only from the base update()
  // guard; the monitor itself never injects additional UART requests.
  const bool monitoring = this->scan_active_;
  if (monitoring) this->scan_active_ = false;
  ToshibaClimateUart::update();
  if (monitoring) this->scan_active_ = true;
}

void ToshibaDiagnosticMonitorUart::parseResponse(std::vector<uint8_t> raw) {
  const int16_t response_register = this->extract_response_register_(raw);

  if (this->scan_active_) {
    const uint32_t now = millis();
    const bool correlated =
        this->monitor_pending_register_ >= 0 &&
        now - this->monitor_pending_since_ <= 1000 &&
        response_register == this->monitor_pending_register_;

    this->log_monitor_rx_(raw, response_register,
                          correlated ? "response" : "unmatched");

    if (correlated) {
      this->monitor_pending_register_ = -1;
      this->monitor_pending_since_ = 0;
    }
  }

  if (raw.size() > 12 && raw[3] == 0x11 &&
      raw[12] == static_cast<uint8_t>(ToshibaRegister::EQUIPMENT_INFO)) {
    const auto equipment = decode_equipment_identification(raw);
    if (!equipment.valid) {
      ESP_LOGW(TAG, "E0 equipment-identification packet did not match the expected class-0x11 layout");
      return;
    }
    ESP_LOGI(TAG, "E0 equipment packet received: IDU model='%s'",
             equipment.idu_model_available ? equipment.idu_model.c_str() : "NULL/unavailable");
    this->set_detected_equipment_(equipment);
    return;
  }

  // Keep the engineering sensors alive, but do not add human-readable decoding
  // to Focused Monitor output itself.
  if (response_register == static_cast<uint8_t>(ToshibaRegister::ODU_STATUS) &&
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

  if (response_register == static_cast<uint8_t>(ToshibaRegister::IDU_STATUS) &&
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
      const uint8_t fan_raw = raw[offset + 2];
      if (fan_raw < 0xFE) {
        this->idu_fan_speed_sensor_->publish_state(fan_raw);
      } else {
        ESP_LOGD(TAG, "E4 IDU fan-speed feedback unavailable (raw=0x%02X)", fan_raw);
      }
    }
    return;
  }

  ToshibaClimateUart::parseResponse(std::move(raw));
}

void ToshibaDiagnosticMonitorUart::set_scan_enabled(bool enabled) {
  if (enabled == this->scan_active_) return;

  this->scan_active_ = enabled;
  this->scan_started_ = false;
  this->scan_request_sent_ = false;
  this->scan_matched_response_ = false;
  this->monitor_stop_requested_ = false;
  this->monitor_pending_register_ = -1;
  this->monitor_pending_since_ = 0;

  if (enabled) {
    ESP_LOGI(TAG, "========== TOSHIBA FOCUSED UART MONITOR STARTED ==========");
    ESP_LOGI(TAG, "raw loiter mode: direction + register (when identifiable) + raw hex");
    ESP_LOGI(TAG, "unparsed timeout/checksum/header failures are retained in the monitor log");
    ESP_LOGI(TAG, "no exploratory register requests are generated by the monitor");
  } else {
    ESP_LOGI(TAG, "========== TOSHIBA FOCUSED UART MONITOR STOPPED ==========");
  }
}

void ToshibaDiagnosticMonitorUart::process_scan_() {
  // Focused Monitor is observational only. Normal polling and user commands
  // continue through their ordinary paths.
}

void ToshibaDiagnosticMonitorUart::on_uart_tx_(const ToshibaCommand &command) {
  if (!this->scan_active_) return;

  const char *relation = "other";
  int16_t reg = -1;

  if (command.operation == ToshibaQueueOperation::HANDSHAKE) {
    relation = "handshake";
  } else if (command.operation == ToshibaQueueOperation::REGISTER) {
    reg = static_cast<uint8_t>(command.register_id);
    if (command.payload.size() > 13 && command.payload[11] == 0x02)
      relation = "write";
    else
      relation = "request";
  }

  ESP_LOGI(TAG, "UART MONITOR TX relation=%s reg=%s len=%u bytes=[%s]",
           relation,
           reg >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(reg)).c_str() : "none",
           static_cast<unsigned>(command.payload.size()),
           format_hex_pretty(command.payload).c_str());

  if (reg >= 0) {
    this->monitor_pending_register_ = reg;
    this->monitor_pending_since_ = millis();
  }
}

void ToshibaDiagnosticMonitorUart::on_uart_rx_unparsed_(
    const std::vector<uint8_t> &raw, const char *reason) {
  if (!this->scan_active_ || raw.empty()) return;

  const int16_t reg = this->extract_response_register_(raw);
  ESP_LOGI(TAG, "UART MONITOR RX relation=unparsed reason=%s reg=%s len=%u bytes=[%s]",
           reason,
           reg >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(reg)).c_str() : "unknown",
           static_cast<unsigned>(raw.size()),
           format_hex_pretty(raw).c_str());
}

void ToshibaDiagnosticMonitorUart::log_monitor_rx_(
    const std::vector<uint8_t> &raw, int16_t reg, const char *relation) const {
  constexpr size_t CHUNK_SIZE = 100;
  const std::string reg_text =
      reg >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(reg)) : "unknown";

  if (raw.size() <= CHUNK_SIZE) {
    ESP_LOGI(TAG, "UART MONITOR RX relation=%s reg=%s len=%u bytes=[%s]",
             relation, reg_text.c_str(), static_cast<unsigned>(raw.size()),
             format_hex_pretty(raw).c_str());
    return;
  }

  const size_t chunks = (raw.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;
  ESP_LOGI(TAG, "UART MONITOR RX relation=%s reg=%s len=%u chunks=%u",
           relation, reg_text.c_str(), static_cast<unsigned>(raw.size()),
           static_cast<unsigned>(chunks));

  for (size_t i = 0; i < chunks; ++i) {
    const size_t first = i * CHUNK_SIZE;
    const size_t last = std::min(first + CHUNK_SIZE, raw.size());
    std::vector<uint8_t> chunk(raw.begin() + first, raw.begin() + last);
    ESP_LOGI(TAG, "UART MONITOR RX [%u/%u] bytes=[%s]",
             static_cast<unsigned>(i + 1), static_cast<unsigned>(chunks),
             format_hex_pretty(chunk).c_str());
  }
}

void ToshibaDiagnosticMonitorUart::log_scan_packet_(const std::vector<uint8_t> &raw) {
  // Retained only for compatibility with the historical scanner entry point.
  // Focused Monitor itself never generates scan traffic.
  this->parseResponse(raw);
}

}  // namespace toshiba_a2a
}  // namespace esphome
