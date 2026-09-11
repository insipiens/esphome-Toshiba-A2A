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
  void set_fan_feedback_sensor(sensor::Sensor *value) { fan_feedback_ = value; }
  void set_cooling_output_sensor(sensor::Sensor *value) { cooling_output_ = value; }
  void set_heating_output_sensor(sensor::Sensor *value) { heating_output_ = value; }
  void set_airflow_sensor(sensor::Sensor *value) { airflow_ = value; }
  void set_heat_exchanger_factor(float value) { heat_exchanger_factor_ = value; }

  void update() override {
    if (climate_ == nullptr)
      return;

    // Fan-only still has a real airflow even though delivered heating/cooling is zero.
    if (climate_->mode == climate::CLIMATE_MODE_FAN_ONLY) {
      float airflow_m3h;
      if (!resolve_airflow_(AirflowMode::COOLING, airflow_m3h)) {
        publish_airflow_invalid_outputs_zero_();
        return;
      }

      if (airflow_ != nullptr) airflow_->publish_state(airflow_m3h);
      if (cooling_output_ != nullptr) cooling_output_->publish_state(0.0f);
      if (heating_output_ != nullptr) heating_output_->publish_state(0.0f);
      return;
    }

    if (climate_->mode != climate::CLIMATE_MODE_HEAT &&
        climate_->mode != climate::CLIMATE_MODE_COOL &&
        climate_->mode != climate::CLIMATE_MODE_DRY &&
        climate_->mode != climate::CLIMATE_MODE_HEAT_COOL) {
      publish_zero_();
      return;
    }

    if (heat_exchanger_temp_ == nullptr) {
      publish_invalid_();
      return;
    }

    const float hx = heat_exchanger_temp_->state;
    const float room = climate_->current_temperature;

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
    } else {  // CLIMATE_MODE_HEAT_COOL
      if (hx >= room) {
        airflow_mode = AirflowMode::HEATING;
        delta_t = hx - room;
        heating = true;
      } else {
        airflow_mode = AirflowMode::COOLING;
        delta_t = room - hx;
      }
    }

    if (delta_t <= 0.0f) {
      publish_zero_();
      return;
    }

    float airflow_m3h;
    if (!resolve_airflow_(airflow_mode, airflow_m3h)) {
      publish_invalid_();
      return;
    }

    const float volume_flow_m3_s = airflow_m3h / 3600.0f;
    const float mass_flow_kg_s = AIR_DENSITY_KG_M3 * volume_flow_m3_s;

    // Sensible thermal output based on model-specific airflow interpretation and
    // the IDU-reported heat-exchanger/room temperature difference.
    const float thermal_w = mass_flow_kg_s * AIR_SPECIFIC_HEAT_J_KG_K * delta_t * heat_exchanger_factor_;

    if (airflow_ != nullptr) airflow_->publish_state(airflow_m3h);
    if (cooling_output_ != nullptr) cooling_output_->publish_state(heating ? 0.0f : thermal_w);
    if (heating_output_ != nullptr) heating_output_->publish_state(heating ? thermal_w : 0.0f);
  }

 protected:
  static constexpr float AIR_DENSITY_KG_M3 = 1.2041f;
  static constexpr float AIR_SPECIFIC_HEAT_J_KG_K = 1005.0f;

  bool resolve_airflow_(AirflowMode mode, float &airflow_m3h) const {
    const std::string &model = climate_->get_idu_model();

    // Where an E4+2 feedback calibration exists for this model, use the actual
    // Toshiba fan/air-velocity feedback. This makes Auto, Quiet and intermediate
    // controller-selected operating points directly usable instead of inferring
    // airflow from the commanded fan setting.
    if (fan_feedback_ != nullptr && std::isfinite(fan_feedback_->state)) {
      if (interpolate_feedback_airflow(model.c_str(), mode, fan_feedback_->state, airflow_m3h))
        return true;
    }

    // Models without a feedback calibration retain the documented fixed/manual
    // fan lookup until their own E4+2-to-airflow relationship is established.
    ManualFanLevel fan_level;
    if (!resolve_manual_fan_(fan_level)) return false;

    const bool hi_power = climate_->is_hi_power_active();
    const auto *airflow_entry = find_manual_airflow(model.c_str(), mode, fan_level, hi_power);
    if (airflow_entry == nullptr) return false;

    airflow_m3h = static_cast<float>(airflow_entry->airflow_m3h);
    return true;
  }

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

  void publish_airflow_invalid_outputs_zero_() {
    if (airflow_ != nullptr) airflow_->publish_state(NAN);
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
  sensor::Sensor *fan_feedback_{nullptr};
  sensor::Sensor *cooling_output_{nullptr};
  sensor::Sensor *heating_output_{nullptr};
  sensor::Sensor *airflow_{nullptr};
  float heat_exchanger_factor_{1.0f};
};

}  // namespace toshiba_output
}  // namespace esphome
