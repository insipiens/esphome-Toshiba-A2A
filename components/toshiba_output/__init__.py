import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER,
    STATE_CLASS_MEASUREMENT,
    UNIT_WATT,
)

CONF_CLIMATE_ID = "climate_id"
CONF_HEAT_EXCHANGER_TEMPERATURE = "heat_exchanger_temperature"
CONF_FAN_FEEDBACK = "fan_feedback"
CONF_COOLING_OUTPUT = "cooling_output"
CONF_HEATING_OUTPUT = "heating_output"
CONF_AIRFLOW = "airflow"
CONF_HEAT_EXCHANGER_FACTOR = "heat_exchanger_factor"

# toshiba_a2a is supplied as a climate platform rather than a top-level
# component. Declaring it in DEPENDENCIES makes ESPHome's component dependency
# validator reject an otherwise valid climate: platform: toshiba_a2a config.
# The C++ estimator still takes the ToshibaClimateUart instance selected by
# climate_id; climate and sensor are the actual top-level dependencies here.
DEPENDENCIES = ["climate", "sensor"]

toshiba_output_ns = cg.esphome_ns.namespace("toshiba_output")
ToshibaOutputEstimator = toshiba_output_ns.class_(
    "ToshibaOutputEstimator", cg.PollingComponent
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ToshibaOutputEstimator),
        cv.Required(CONF_CLIMATE_ID): cv.use_id(climate.Climate),
        cv.Required(CONF_HEAT_EXCHANGER_TEMPERATURE): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_FAN_FEEDBACK): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_COOLING_OUTPUT): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HEATING_OUTPUT): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AIRFLOW): sensor.sensor_schema(
            unit_of_measurement="m³/h",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # Ratio between the IDU heat-exchanger thermistor delta-T and the
        # effective leaving-air delta-T. Start at 1.0 for comparison testing,
        # then fit from simultaneous HX/outlet-air measurements.
        cv.Optional(CONF_HEAT_EXCHANGER_FACTOR, default=1.0): cv.positive_float,
    }
).extend(cv.polling_component_schema("5s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    climate_var = await cg.get_variable(config[CONF_CLIMATE_ID])
    hx_var = await cg.get_variable(config[CONF_HEAT_EXCHANGER_TEMPERATURE])

    cg.add(var.set_climate(climate_var))
    cg.add(var.set_heat_exchanger_temperature_sensor(hx_var))
    cg.add(var.set_heat_exchanger_factor(config[CONF_HEAT_EXCHANGER_FACTOR]))

    if CONF_FAN_FEEDBACK in config:
        fan_var = await cg.get_variable(config[CONF_FAN_FEEDBACK])
        cg.add(var.set_fan_feedback_sensor(fan_var))

    if CONF_COOLING_OUTPUT in config:
        sens = await sensor.new_sensor(config[CONF_COOLING_OUTPUT])
        cg.add(var.set_cooling_output_sensor(sens))

    if CONF_HEATING_OUTPUT in config:
        sens = await sensor.new_sensor(config[CONF_HEATING_OUTPUT])
        cg.add(var.set_heating_output_sensor(sens))

    if CONF_AIRFLOW in config:
        sens = await sensor.new_sensor(config[CONF_AIRFLOW])
        cg.add(var.set_airflow_sensor(sens))
