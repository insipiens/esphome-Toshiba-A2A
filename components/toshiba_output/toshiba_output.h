#pragma once

#include <cmath>
#include <cstring>
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/toshiba_suzumi/toshiba_climate.h"
#include "toshiba_airflow_data.h"

namespace esphome {
namespace toshiba_output {

class ToshibaOutputEstimator : public PollingComponent {
 public:
  void set_climate(toshiba_suzumi::ToshibaClimateUart *value) { climate_ = value; }
  void set_heat_exchanger_temperature_sensor(sensor::Sensor *value) { heat_exchanger_temp_ = value; }
  void set_cooling_output_sensor(sensor::Sensor *value) { cooling_output_ = value; }
  void set_heating_output_sensor(sensor::Sensor *value) { heating_output_ = value; }
  void set_airflow_sensor(sensor::Sensor *value) { airflow_ = value; }
  void set_heat_exchanger_factor(float value) { heat_exchanger_factor_ = value; }

  void update() override {
    if (climate_ == nullptr || heat_exchanger_temp_ == nullptr)
      return;

    const float hx = heat_exchanger_temp_->state;
    const float room = climate_->current_temperature;  // Toshiba 0xBB room/return-air temperature.

    if (!std::isfinite(hx) || !std::isfinite(room)) {
      publish_invalid_();
      return;
    }

    AirflowMode airflow_mode;
    float delta_t = 0.0f;
    bool heating = false;

    if (climate_->mode == climate::CLIMATE_MODE_HEAT) {
      airflow_mode = AirflowMode::HEATING;
      delta_t = hx - room;
      heating = true;
    } else if (climate_->mode == climate::CLIMATE_MODE_COOL ||
               climate_->mode == climate::CLIMATE_MODE_DRY) {
      airflow_mode = AirflowMode::COOLING;
      delta_t = room - hx;
    } else if (climate_->mode == climate::CLIMATE_MODE_HEAT_COOL) {
      if (hx >= room) {
        airflow_mode = AirflowMode::HEATING;
        delta_t = hx - room;
        heating = true;
      } else {
        airflow_mode = AirflowMode::COOLING;
        delta_t = room - hx;
      }
    } else {
      publish_zero_();
      return;
    }

    if (delta_t <= 0.0f) {
      publish_zero_();
      return;
    }

    ManualFanLevel fan_level;
    if (!resolve_manual_fan_(fan_level)) {
      // Auto and Quiet do not correspond to one fixed manufacturer airflow.
      // E4 actual-fan feedback can be used here once its W1..WF mapping is proven.
      publish_invalid_();
      return;
    }

    const std::string &model = climate_->get_idu_model();
    const bool hi_power = climate_->is_hi_power_active();
    const auto *airflow_entry = find_manual_airflow(model.c_str(), airflow_mode, fan_level, hi_power);
    if (airflow_entry == nullptr) {
      publish_invalid_();
      return;
    }

    const float volume_flow_m3_s = static_cast<float>(airflow_entry->airflow_m3h) / 3600.0f;
    const float mass_flow_kg_s = AIR_DENSITY_KG_M3 * volume_flow_m3_s;

    // Sensible thermal output based on manufacturer airflow and the IDU-reported
    // heat-exchanger/room temperature difference. heat_exchanger_factor allows
    // calibration of HX thermistor temperature against actual leaving-air delta-T.
    const float thermal_w = mass_flow_kg_s * AIR_SPECIFIC_HEAT_J_KG_K * delta_t * heat_exchanger_factor_;

    if (airflow_ != nullptr) airflow_->publish_state(airflow_entry->airflow_m3h);
    if (cooling_output_ != nullptr) cooling_output_->publish_state(heating ? 0.0f : thermal_w);
    if (heating_output_ != nullptr) heating_output_->publish_state(heating ? thermal_w : 0.0f);
  }

 protected:
  static constexpr float AIR_DENSITY_KG_M3 = 1.2041f;
  static constexpr float AIR_SPECIFIC_HEAT_J_KG_K = 1005.0f;

  bool resolve_manual_fan_(ManualFanLevel &level) const {
    if (climate_->has_custom_fan_mode()) {
      const auto custom = climate_->get_custom_fan_mode();
      if (std::strcmp(custom.c_str(), toshiba_suzumi::CUSTOM_FAN_LEVEL_2) == 0) {
        level = ManualFanLevel::LOW_MEDIUM;
        return true;
      }
      if (std::strcmp(custom.c_str(), toshiba_suzumi::CUSTOM_FAN_LEVEL_4) == 0) {
        level = ManualFanLevel::MEDIUM_HIGH;
        return true;
      }
      return false;
    }

    if (!climate_->fan_mode.has_value()) return false;

    switch (*climate_->fan_mode) {
      case climate::CLIMATE_FAN_LOW:
        level = ManualFanLevel::LOW;
        return true;
      case climate::CLIMATE_FAN_MEDIUM:
        level = ManualFanLevel::MEDIUM;
        return true;
      case climate::CLIMATE_FAN_HIGH:
        level = ManualFanLevel::HIGH;
        return true;
      default:
        return false;
    }
  }

  void publish_zero_() {
    if (airflow_ != nullptr) airflow_->publish_state(0.0f);
    if (cooling_output_ != nullptr) cooling_output_->publish_state(0.0f);
    if (heating_output_ != nullptr) heating_output_->publish_state(0.0f);
  }

  void publish_invalid_() {
    if (airflow_ != nullptr) airflow_->publish_state(NAN);
    if (cooling_output_ != nullptr) cooling_output_->publish_state(NAN);
    if (heating_output_ != nullptr) heating_output_->publish_state(NAN);
  }

  toshiba_suzumi::ToshibaClimateUart *climate_{nullptr};
  sensor::Sensor *heat_exchanger_temp_{nullptr};
  sensor::Sensor *cooling_output_{nullptr};
  sensor::Sensor *heating_output_{nullptr};
  sensor::Sensor *airflow_{nullptr};
  float heat_exchanger_factor_{1.0f};
};

}  // namespace toshiba_output
}  // namespace esphome
