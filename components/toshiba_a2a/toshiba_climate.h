#pragma once

#include <array>

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "toshiba_climate_mode.h"
#include "toshiba_device_profile.h"
#include "toshiba_identity.h"
#include "toshiba_registers.h"

namespace esphome {
namespace time {
class RealTimeClock;
}  // namespace time
namespace toshiba_a2a {

static const char *const TAG = "ToshibaClimateUart";
static const uint8_t MAX_TEMP = 30;
static const uint8_t MIN_TEMP_STANDARD = 17;
static const uint8_t SPECIAL_TEMP_OFFSET = 16;
static const uint8_t SPECIAL_MODE_EIGHT_DEG_MIN_TEMP = 5;
static const uint8_t SPECIAL_MODE_EIGHT_DEG_MAX_TEMP = 13;
static const uint8_t SPECIAL_MODE_EIGHT_DEG_DEF_TEMP = 8;
static const uint8_t NORMAL_MODE_DEF_TEMP = 20;

static const std::vector<uint8_t> HANDSHAKE[6] = {
    {2, 255, 255, 0, 0, 0, 0, 2},       {2, 255, 255, 1, 0, 0, 1, 2, 254}, {2, 0, 0, 0, 0, 0, 2, 2, 2, 250},
    {2, 0, 1, 129, 1, 0, 2, 0, 0, 123}, {2, 0, 1, 2, 0, 0, 2, 0, 0, 254},  {2, 0, 2, 0, 0, 0, 0, 254},
};

static const std::vector<uint8_t> AFTER_HANDSHAKE[2] = {
    {2, 0, 2, 1, 0, 0, 2, 0, 0, 251},
    {2, 0, 2, 2, 0, 0, 2, 0, 0, 250},
};

struct ToshibaCommand {
  ToshibaQueueOperation operation{ToshibaQueueOperation::REGISTER};
  ToshibaRegister register_id{ToshibaRegister::POWER_STATE};
  std::vector<uint8_t> payload;
  int delay{0};
};

class ToshibaValidatedFunctionSwitch;
class ToshibaValidatedSilentSelect;
class ToshibaValidatedSpecialModeLevelSelect;
class ToshibaValidatedPowerSelect;
class ToshibaPureSwitch;
class ToshibaDefrostButton;
class ToshibaHorizontalAirDirectionSelect;
class ToshibaValidatedVerticalAirDirectionSelect;

class ToshibaClimateUart : public PollingComponent, public climate::Climate, public uart::UARTDevice {
 public:
  ToshibaClimateUart();
  void setup() override;
  void loop() override;
  void dump_config() override;
  void update() override;
  virtual void scan();
  virtual void set_scan_enabled(bool enabled) { if (enabled) this->scan(); }
  virtual bool is_scan_enabled() const { return this->scan_active_; }
  void set_wifi_led(bool enabled);
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_indoor_temp_sensor(sensor::Sensor *sensor) { indoor_temp_sensor_ = sensor; }
  void set_outdoor_temp_sensor(sensor::Sensor *sensor) { outdoor_temp_sensor_ = sensor; }
  void set_odu_discharge_temp_sensor(sensor::Sensor *sensor) { odu_discharge_temp_sensor_ = sensor; }
  void set_odu_suction_temp_sensor(sensor::Sensor *sensor) { odu_suction_temp_sensor_ = sensor; }
  void set_odu_heat_exchanger_temp_sensor(sensor::Sensor *sensor) { odu_heat_exchanger_temp_sensor_ = sensor; }
  void set_compressor_load_sensor(sensor::Sensor *sensor) { compressor_load_sensor_ = sensor; }
  void set_compressor_current_sensor(sensor::Sensor *sensor) { compressor_current_sensor_ = sensor; }
  void set_idu_heat_exchanger_temp_sensor(sensor::Sensor *sensor) { idu_heat_exchanger_temp_sensor_ = sensor; }
  void set_idu_junction_temp_sensor(sensor::Sensor *sensor) { idu_junction_temp_sensor_ = sensor; }
  void set_idu_fan_speed_sensor(sensor::Sensor *sensor) { idu_fan_speed_sensor_ = sensor; }
  void set_register_90_raw_sensor(sensor::Sensor *sensor) { register_90_raw_sensor_ = sensor; }
  void set_register_94_raw_sensor(sensor::Sensor *sensor) { register_94_raw_sensor_ = sensor; }
  void set_register_c7_raw_sensor(sensor::Sensor *sensor) { register_c7_raw_sensor_ = sensor; }
  void set_idu_model_sensor(text_sensor::TextSensor *sensor) { idu_model_sensor_ = sensor; }
  void set_idu_identity_1_sensor(text_sensor::TextSensor *sensor) { idu_identity_1_sensor_ = sensor; }
  void set_idu_identity_2_sensor(text_sensor::TextSensor *sensor) { idu_identity_2_sensor_ = sensor; }
  void set_idu_identity_3_sensor(text_sensor::TextSensor *sensor) { idu_identity_3_sensor_ = sensor; }
  void set_odu_model_sensor(text_sensor::TextSensor *sensor) { odu_model_sensor_ = sensor; }
  void set_odu_identity_1_sensor(text_sensor::TextSensor *sensor) { odu_identity_1_sensor_ = sensor; }
  void set_odu_identity_2_sensor(text_sensor::TextSensor *sensor) { odu_identity_2_sensor_ = sensor; }
  void set_odu_identity_3_sensor(text_sensor::TextSensor *sensor) { odu_identity_3_sensor_ = sensor; }

  // model_override is a static fallback for units whose E0 IDU model field is
  // blank/NULL. A valid Toshiba-reported model remains the diagnostic truth.
  void set_model_override(const std::string &model) {
    if (model.empty()) return;
    this->model_override_ = model;
    if (this->idu_model_.empty()) {
      this->family_profile_ = &profile_for_model(model);
    }
  }
  const std::string &get_idu_model() const {
    return this->idu_model_.empty() ? this->model_override_ : this->idu_model_;
  }
  const std::string &get_reported_idu_model() const { return this->idu_model_; }
  const std::string &get_model_override() const { return this->model_override_; }

  void restore_idu_model(const std::string &model) {
    if (model.empty()) return;
    this->idu_model_ = model;
    this->family_profile_ = &profile_for_model(model);
    if (this->idu_model_sensor_ != nullptr) this->idu_model_sensor_->publish_state(model);
    // Persisted IDU models are written only from accepted Toshiba E0 reports.
    // Re-apply the same optional-entity gate during startup so discovery is
    // complete before Home Assistant reconnects on subsequent boots.
  }
  void restore_idu_identity_1(const std::string &value) {
    if (value.empty()) return;
    this->idu_identity_1_ = value;
    if (this->idu_identity_1_sensor_ != nullptr) this->idu_identity_1_sensor_->publish_state(value);
  }
  void restore_idu_identity_2(const std::string &value) {
    if (value.empty()) return;
    this->idu_identity_2_ = value;
    if (this->idu_identity_2_sensor_ != nullptr) this->idu_identity_2_sensor_->publish_state(value);
  }
  void restore_idu_identity_3(const std::string &value) {
    if (value.empty()) return;
    this->idu_identity_3_ = value;
    if (this->idu_identity_3_sensor_ != nullptr) this->idu_identity_3_sensor_->publish_state(value);
  }
  void restore_odu_model(const std::string &model) {
    if (model.empty()) return;
    this->odu_model_ = model;
    if (this->odu_model_sensor_ != nullptr) this->odu_model_sensor_->publish_state(model);
  }
  void restore_odu_identity_1(const std::string &value) {
    if (value.empty()) return;
    this->odu_identity_1_ = value;
    if (this->odu_identity_1_sensor_ != nullptr) this->odu_identity_1_sensor_->publish_state(value);
  }
  void restore_odu_identity_2(const std::string &value) {
    if (value.empty()) return;
    this->odu_identity_2_ = value;
    if (this->odu_identity_2_sensor_ != nullptr) this->odu_identity_2_sensor_->publish_state(value);
  }
  void restore_odu_identity_3(const std::string &value) {
    if (value.empty()) return;
    this->odu_identity_3_ = value;
    if (this->odu_identity_3_sensor_ != nullptr) this->odu_identity_3_sensor_->publish_state(value);
  }
  optional<SPECIAL_MODE> get_special_mode() const { return special_mode_; }
  bool is_hi_power_active() const { return special_mode_.has_value() && special_mode_.value() == SPECIAL_MODE::HI_POWER; }
  void set_time(time::RealTimeClock *time) { time_ = time; }
  void set_energy_sensor(sensor::Sensor *sensor) { energy_sensor_ = sensor; }
  void set_power_sensor(sensor::Sensor *sensor) { power_sensor_ = sensor; }
  void set_pwr_select(select::Select *pws_select) { pwr_select_ = pws_select; }
  void set_vertical_air_direction_select(select::Select *sel) { vertical_air_direction_select_ = sel; }
  void set_self_clean_sensor(binary_sensor::BinarySensor *sensor) { self_clean_sensor_ = sensor; }
  void set_horizontal_swing(bool enabled) { horizontal_swing_ = enabled; }
  void disable_heat_mode(bool disabled) { heat_mode_disabled_ = disabled; }
  void disable_wifi_led(bool disabled) { wifi_led_disabled_ = disabled; }
  void set_supported_presets(const std::vector<const char *> &presets) { supported_presets_ = presets; }
  void set_min_temp(uint8_t min_temp) { min_temp_ = min_temp; }
  void set_time_sync_interval(uint32_t interval) { time_sync_interval_ = interval; }

 protected:
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  std::vector<uint8_t> rx_message_;
  std::vector<ToshibaCommand> command_queue_;
  uint32_t last_command_timestamp_ = 0;
  uint32_t last_rx_char_timestamp_ = 0;
  STATE power_state_ = STATE::OFF;
  bool self_clean_running_ = false;
  optional<SPECIAL_MODE> special_mode_ = SPECIAL_MODE::STANDARD;
  select::Select *pwr_select_ = nullptr;
  select::Select *vertical_air_direction_select_ = nullptr;
  binary_sensor::BinarySensor *self_clean_sensor_ = nullptr;
  sensor::Sensor *indoor_temp_sensor_ = nullptr;
  sensor::Sensor *outdoor_temp_sensor_ = nullptr;
  sensor::Sensor *odu_discharge_temp_sensor_ = nullptr;
  sensor::Sensor *odu_suction_temp_sensor_ = nullptr;
  sensor::Sensor *odu_heat_exchanger_temp_sensor_ = nullptr;
  sensor::Sensor *compressor_load_sensor_ = nullptr;
  sensor::Sensor *compressor_current_sensor_ = nullptr;
  sensor::Sensor *idu_heat_exchanger_temp_sensor_ = nullptr;
  sensor::Sensor *idu_junction_temp_sensor_ = nullptr;
  sensor::Sensor *idu_fan_speed_sensor_ = nullptr;
  sensor::Sensor *register_90_raw_sensor_ = nullptr;
  sensor::Sensor *register_94_raw_sensor_ = nullptr;
  sensor::Sensor *register_c7_raw_sensor_ = nullptr;
  text_sensor::TextSensor *idu_model_sensor_ = nullptr;
  text_sensor::TextSensor *idu_identity_1_sensor_ = nullptr;
  text_sensor::TextSensor *idu_identity_2_sensor_ = nullptr;
  text_sensor::TextSensor *idu_identity_3_sensor_ = nullptr;
  text_sensor::TextSensor *odu_model_sensor_ = nullptr;
  text_sensor::TextSensor *odu_identity_1_sensor_ = nullptr;
  text_sensor::TextSensor *odu_identity_2_sensor_ = nullptr;
  text_sensor::TextSensor *odu_identity_3_sensor_ = nullptr;
  time::RealTimeClock *time_ = nullptr;
  sensor::Sensor *energy_sensor_ = nullptr;
  sensor::Sensor *power_sensor_ = nullptr;

  const ToshibaFamilyProfile *family_profile_{&UNKNOWN_FAMILY_PROFILE};
  std::string model_override_;
  std::string idu_model_;
  std::string idu_identity_1_;
  std::string idu_identity_2_;
  std::string idu_identity_3_;
  std::string odu_model_;
  std::string odu_identity_1_;
  std::string odu_identity_2_;
  std::string odu_identity_3_;

  bool horizontal_swing_ = false;
  uint8_t min_temp_ = 17;
  bool heat_mode_disabled_ = false;
  bool wifi_led_disabled_ = false;
  std::vector<const char *> supported_presets_;
  uint32_t last_time_sync_ = 0;
  uint32_t last_energy_sync_ = 0;
  uint32_t last_total_daily_energy_ = 0;
  uint32_t last_energy_update_ms_ = 0;
  uint16_t daily_energy_usage_[24] = {0};
  bool time_synced_ = false;
  uint32_t time_sync_interval_{86400000};

  bool scan_active_ = false;
  bool scan_started_ = false;
  bool scan_request_sent_ = false;
  bool scan_matched_response_ = false;
  uint8_t scan_register_ = 0x80;
  uint32_t scan_register_started_ = 0;
  uint32_t scan_last_packet_timestamp_ = 0;
  uint16_t scan_tested_ = 0;
  uint16_t scan_matched_ = 0;
  uint16_t scan_no_match_ = 0;

  void enqueue_command_(const ToshibaCommand &command);
  void send_to_uart(const ToshibaCommand command);
  virtual void on_uart_tx_(const ToshibaCommand &command) {}
  void start_handshake();
  virtual void parseResponse(std::vector<uint8_t> rawData);
  void requestData(ToshibaRegister cmd);
  void process_command_queue_();
  void sendCmd(ToshibaRegister cmd, uint8_t value);
  void getInitData();
  void handle_rx_byte_(uint8_t c);
  bool validate_message_();
  void set_self_clean_running_(bool running);
  void configure_supported_custom_modes_();
  void set_detected_equipment_(const ToshibaEquipmentIdentification &equipment);
  virtual void process_scan_();
  void send_scan_request_();
  void complete_scan_register_();
  void finish_scan_();
  virtual void log_scan_packet_(const std::vector<uint8_t> &raw_data);
  int16_t extract_response_register_(const std::vector<uint8_t> &raw_data) const;
  void log_scan_ascii_(const std::vector<uint8_t> &raw_data) const;
#ifdef USE_TIME
  void check_time_sync_(uint32_t now);
  void sync_time_();
#endif
  void sync_energy_();
  void estimate_wattage_(uint32_t current_energy);

};

class ToshibaDiagnosticMonitorUart : public ToshibaClimateUart {
 public:
  void update() override;
  void scan() override { this->set_scan_enabled(true); }
  void set_scan_enabled(bool enabled) override;
  bool is_scan_enabled() const override { return this->scan_active_ && !this->monitor_stop_requested_; }

 protected:
  void parseResponse(std::vector<uint8_t> raw_data) override;
  void process_scan_() override;
  void log_scan_packet_(const std::vector<uint8_t> &raw_data) override;

 protected:
  void on_uart_tx_(const ToshibaCommand &command) override;

 private:
  bool monitor_stop_requested_ = false;
  int16_t monitor_pending_register_{-1};
  uint32_t monitor_pending_since_{0};
  bool monitor_pending_write_{false};
  bool monitor_pending_has_value_{false};
  uint8_t monitor_pending_value_{0};
  std::array<std::vector<uint8_t>, 128> monitor_last_payload_{};
  std::array<bool, 128> monitor_payload_seen_{};

  void log_monitor_bytes_(const std::vector<uint8_t> &raw_data, int16_t response_register) const;
  void log_monitor_decoded_(const std::vector<uint8_t> &raw_data, int16_t response_register,
                            bool correlated);
  bool extract_monitor_payload_(const std::vector<uint8_t> &raw_data, int16_t response_register,
                                std::vector<uint8_t> &payload) const;
  void remember_monitor_payload_(uint8_t response_register, const std::vector<uint8_t> &payload);
  void log_timer_bank_snapshot_() const;
};

class ToshibaValidatedControlUart : public ToshibaDiagnosticMonitorUart {
 public:
  void setup() override;
  void set_horizontal_air_direction_select(select::Select *sel) { horizontal_air_direction_select_ = sel; }
  void set_pure_switch(ToshibaPureSwitch *entity) { pure_switch_ = entity; }
  void set_defrost_active_sensor(binary_sensor::BinarySensor *sensor) { defrost_active_sensor_ = sensor; }
  void set_eco_switch(ToshibaValidatedFunctionSwitch *entity) { validated_eco_switch_ = entity; }
  void set_hi_power_switch(ToshibaValidatedFunctionSwitch *entity) { validated_hi_power_switch_ = entity; }
  void set_eight_degree_heat_switch(ToshibaValidatedFunctionSwitch *entity) { validated_eight_degree_heat_switch_ = entity; }
  void set_sleep_switch(ToshibaValidatedFunctionSwitch *entity) { validated_sleep_switch_ = entity; }
  void set_floor_switch(ToshibaValidatedFunctionSwitch *entity) { validated_floor_switch_ = entity; }
  void set_comfort_switch(ToshibaValidatedFunctionSwitch *entity) { validated_comfort_switch_ = entity; }
  void set_fireplace_select(ToshibaValidatedSpecialModeLevelSelect *entity) { validated_fireplace_select_ = entity; }
  void set_outdoor_silent_select(ToshibaValidatedSilentSelect *entity) { validated_outdoor_silent_select_ = entity; }
  void set_start_defrost_button(ToshibaDefrostButton *entity) { start_defrost_button_ = entity; }
  void set_strong_defrost_button(ToshibaDefrostButton *entity) { strong_defrost_button_ = entity; }
  void set_disabled_features(uint32_t features) { disabled_features_ = features; }

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void parseResponse(std::vector<uint8_t> raw_data) override;
  ToshibaHvacMode current_hvac_mode_() const;
  bool validated_function_allowed_(ToshibaFeature feature, ToshibaHvacMode mode) const;
  bool validated_fan_allowed_(uint8_t fan_option, ToshibaHvacMode mode) const;
  bool family_supports_feature_(ToshibaFeature feature) const;
  bool feature_disabled_(ToshibaFeature feature) const;
  bool effective_feature_available_(ToshibaFeature feature) const;
  void apply_effective_capabilities_();
  void clear_validated_f7_entities_();
  void publish_validated_f7_mode_(SPECIAL_MODE mode);
  void on_set_validated_special_mode_(SPECIAL_MODE mode, bool enabled);
  void on_set_validated_silent_(const std::string &value);
  void on_set_validated_power_level_(const std::string &value);
  void on_set_pure_(bool enabled);
  void on_press_defrost_(bool strong);
  void on_set_vertical_fixed_position_(const std::string &value);
  void on_set_horizontal_air_direction_(const std::string &value);
  void publish_packed_fix_state_(uint8_t raw);
  void publish_horizontal_air_direction_(uint8_t raw);

  select::Select *horizontal_air_direction_select_ = nullptr;
  ToshibaPureSwitch *pure_switch_ = nullptr;
  binary_sensor::BinarySensor *defrost_active_sensor_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_eco_switch_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_hi_power_switch_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_eight_degree_heat_switch_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_sleep_switch_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_floor_switch_ = nullptr;
  ToshibaValidatedFunctionSwitch *validated_comfort_switch_ = nullptr;
  ToshibaValidatedSpecialModeLevelSelect *validated_fireplace_select_ = nullptr;
  ToshibaValidatedSilentSelect *validated_outdoor_silent_select_ = nullptr;
  ToshibaDefrostButton *start_defrost_button_ = nullptr;
  ToshibaDefrostButton *strong_defrost_button_ = nullptr;
  uint32_t disabled_features_{FEATURE_NONE};

  uint8_t fix_horizontal_index_{1};
  uint8_t fix_vertical_index_{1};
  bool have_packed_fix_state_{false};

  friend class ToshibaValidatedFunctionSwitch;
  friend class ToshibaValidatedSilentSelect;
  friend class ToshibaValidatedSpecialModeLevelSelect;
  friend class ToshibaValidatedPowerSelect;
  friend class ToshibaPureSwitch;
  friend class ToshibaDefrostButton;
  friend class ToshibaHorizontalAirDirectionSelect;
  friend class ToshibaValidatedVerticalAirDirectionSelect;
};

class ToshibaValidatedVerticalAirDirectionSelect : public select::Select,
                                                    public esphome::Parented<ToshibaValidatedControlUart> {
 protected:
  void control(const std::string &value) override;
};

class ToshibaValidatedFunctionSwitch : public switch_::Switch,
                                       public esphome::Parented<ToshibaValidatedControlUart> {
 public:
  void set_special_mode(uint8_t mode) { mode_ = static_cast<SPECIAL_MODE>(mode); }
 protected:
  void write_state(bool state) override;
  SPECIAL_MODE mode_{SPECIAL_MODE::STANDARD};
};

class ToshibaValidatedSilentSelect : public select::Select,
                                     public esphome::Parented<ToshibaValidatedControlUart> {
 protected:
  void control(const std::string &value) override;
};
class ToshibaValidatedSpecialModeLevelSelect : public select::Select,
                                              public esphome::Parented<ToshibaValidatedControlUart> {
 public:
  void set_special_modes(uint8_t one,uint8_t two){level_one_=static_cast<SPECIAL_MODE>(one);level_two_=static_cast<SPECIAL_MODE>(two);}
  void set_option_names(const std::string &one,const std::string &two){option_one_=one;option_two_=two;}
 protected:
  void control(const std::string &value) override;
  SPECIAL_MODE level_one_{SPECIAL_MODE::STANDARD},level_two_{SPECIAL_MODE::STANDARD};
  std::string option_one_,option_two_;
};

class ToshibaValidatedPowerSelect : public select::Select,
                                    public esphome::Parented<ToshibaValidatedControlUart> {
 protected:
  void control(const std::string &value) override;
};

class ToshibaPureSwitch : public switch_::Switch, public esphome::Parented<ToshibaValidatedControlUart> {
 protected:
  void write_state(bool state) override;
};

class ToshibaDefrostButton : public button::Button, public esphome::Parented<ToshibaValidatedControlUart> {
 public:
  void set_strong(bool strong) { strong_ = strong; }
 protected:
  void press_action() override;
  bool strong_{false};
};

class ToshibaHorizontalAirDirectionSelect : public select::Select,
                                             public esphome::Parented<ToshibaValidatedControlUart> {
 protected:
  void control(const std::string &value) override;
};

}  // namespace toshiba_a2a
}  // namespace esphome