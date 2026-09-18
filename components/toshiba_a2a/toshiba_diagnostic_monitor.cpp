#include <algorithm>
#include <utility>
#include "toshiba_climate.h"
#include "toshiba_device_profile.h"
#include "toshiba_identity.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace toshiba_a2a {

static constexpr size_t CHUNK = 24;

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
    const bool pending_fresh =
        this->monitor_pending_register_ >= 0 &&
        now - this->monitor_pending_since_ <= 1000;
    const bool correlated =
        pending_fresh && response_register == this->monitor_pending_register_;

    const char *source =
        correlated ? (this->monitor_pending_write_ ? "write-response" : "request-response")
                   : "unsolicited";
    const std::string tx_value =
        correlated && this->monitor_pending_has_value_
            ? str_sprintf(" tx_value=0x%02X", static_cast<unsigned>(this->monitor_pending_value_))
            : "";

    ESP_LOGI(TAG,
             "UART MONITOR RX source=%s class=0x%02X reg=%s length=%u checksum=OK%s%s",
             source,
             raw.size() > 3 ? static_cast<unsigned>(raw[3]) : 0U,
             response_register >= 0
                 ? str_sprintf("0x%02X", static_cast<unsigned>(response_register)).c_str()
                 : "unknown",
             static_cast<unsigned>(raw.size()),
             correlated
                 ? str_sprintf(" latency=%ums", static_cast<unsigned>(now - this->monitor_pending_since_)).c_str()
                 : "",
             tx_value.c_str());

    this->log_monitor_bytes_(raw, response_register);
    this->log_monitor_decoded_(raw, response_register, correlated);

    if (correlated) {
      this->monitor_pending_register_ = -1;
      this->monitor_pending_since_ = 0;
      this->monitor_pending_write_ = false;
      this->monitor_pending_has_value_ = false;
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

  // Suppress the temporary verbose E4/E5 base dumps while retaining sensors.
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
  this->monitor_pending_write_ = false;
  this->monitor_pending_has_value_ = false;

  if (enabled) {
    this->monitor_last_payload_.fill({});
    this->monitor_payload_seen_.fill(false);
    ESP_LOGI(TAG, "========== TOSHIBA FOCUSED UART MONITOR STARTED ==========");
    ESP_LOGI(TAG, "loiter mode: observes normal board TX, IDU RX and unsolicited state publications");
    ESP_LOGI(TAG, "no exploratory register requests are generated by the monitor");
  } else {
    ESP_LOGI(TAG, "========== TOSHIBA FOCUSED UART MONITOR STOPPED ==========");
  }
}

void ToshibaDiagnosticMonitorUart::process_scan_() {
  // Loiter monitor is observational only. Normal polling and user commands
  // continue through their ordinary paths.
}

void ToshibaDiagnosticMonitorUart::on_uart_tx_(const ToshibaCommand &command) {
  if (!this->scan_active_) return;

  const char *kind = "other";
  int16_t reg = -1;
  bool has_value = false;
  uint8_t value = 0;

  if (command.operation == ToshibaQueueOperation::HANDSHAKE) {
    kind = "handshake";
  } else if (command.operation == ToshibaQueueOperation::REGISTER &&
             command.payload.size() > 12) {
    reg = command.payload[12];
    if (command.payload.size() > 13 && command.payload[11] == 0x02) {
      kind = "write";
      has_value = true;
      value = command.payload[13];
    } else {
      kind = "request";
    }
  }

  ESP_LOGI(TAG, "UART MONITOR TX kind=%s reg=%s%s bytes=[%s]",
           kind,
           reg >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(reg)).c_str() : "none",
           has_value ? str_sprintf(" value=0x%02X", static_cast<unsigned>(value)).c_str() : "",
           format_hex_pretty(command.payload).c_str());

  if (reg >= 0) {
    this->monitor_pending_register_ = reg;
    this->monitor_pending_since_ = millis();
    this->monitor_pending_write_ = has_value;
    this->monitor_pending_has_value_ = has_value;
    this->monitor_pending_value_ = value;
  }
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

void ToshibaDiagnosticMonitorUart::log_scan_packet_(const std::vector<uint8_t> &raw) {
  // Retained for compatibility with legacy scanner call paths. Passive focused
  // monitor operation reaches parseResponse() directly.
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

void ToshibaDiagnosticMonitorUart::log_monitor_decoded_(const std::vector<uint8_t> &raw, int16_t reg,
                                                         bool correlated) {
  if (raw.size() == 16) {
    ESP_LOGI(TAG, "UART MONITOR ACK source=%s reg=%s bytes=[%s]",
             correlated ? "correlated" : "unsolicited",
             reg >= 0 ? str_sprintf("0x%02X", static_cast<unsigned>(reg)).c_str() : "unknown",
             format_hex_pretty(raw).c_str());
    return;
  }

  std::vector<uint8_t> payload;
  if (!this->extract_monitor_payload_(raw, reg, payload)) return;

  bool changed = true;
  if (reg >= 0x80) {
    const size_t index = static_cast<size_t>(reg - 0x80);
    if (index < this->monitor_last_payload_.size() && this->monitor_payload_seen_[index])
      changed = this->monitor_last_payload_[index] != payload;
  }

  this->remember_monitor_payload_(static_cast<uint8_t>(reg), payload);

  const char *source = correlated ? "response" : "unsolicited";

  if (changed) {
    ESP_LOGI(TAG, "UART MONITOR CHANGE source=%s reg=0x%02X value=[%s] len=%u%s",
             source, static_cast<unsigned>(reg), format_hex_pretty(payload).c_str(),
             static_cast<unsigned>(payload.size()),
             correlated ? "" : " remote/control-origin candidate");
  } else {
    ESP_LOGD(TAG, "UART MONITOR VALUE source=%s reg=0x%02X value=[%s] len=%u unchanged",
             source, static_cast<unsigned>(reg), format_hex_pretty(payload).c_str(),
             static_cast<unsigned>(payload.size()));
  }
}

}  // namespace toshiba_a2a
}  // namespace esphome
