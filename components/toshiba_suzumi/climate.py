import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, button, sensor, climate, uart, select, switch, text_sensor
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_AMPERE,
    UNIT_WATT_HOURS,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_RUNNING,
    DEVICE_CLASS_ENERGY,
    CONF_TIME_ID,
    STATE_CLASS_TOTAL_INCREASING,
    __version__ as ESPHOME_VERSION
)
from packaging import version
import logging

_LOGGER = logging.getLogger(__name__)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor", "button", "sensor", "select", "switch", "text_sensor"]

CONF_ROOM_TEMP = "room_temp"
CONF_INDOOR_TEMP = "indoor_temp"
CONF_OUTDOOR_TEMP = "outdoor_temp"
CONF_ODU_DISCHARGE_TEMP = "odu_discharge_temp"
CONF_ODU_SUCTION_TEMP = "odu_suction_temp"
CONF_ODU_HEAT_EXCHANGER_TEMP = "odu_heat_exchanger_temp"
CONF_COMPRESSOR_LOAD = "compressor_load"
CONF_COMPRESSOR_CURRENT = "compressor_current"
CONF_IDU_HEAT_EXCHANGER_TEMP = "idu_heat_exchanger_temp"
CONF_IDU_JUNCTION_TEMP = "idu_junction_temp"
CONF_IDU_FAN_SPEED = "idu_fan_speed"
CONF_REGISTER_90_RAW = "register_90_raw"
CONF_REGISTER_94_RAW = "register_94_raw"
CONF_REGISTER_C7_RAW = "register_c7_raw"
CONF_IDU_MODEL = "idu_model"
CONF_ODU_MODEL = "odu_model"
CONF_PWR_SELECT = "power_select"
CONF_VERTICAL_AIR_DIRECTION = "vertical_air_direction"
CONF_HORIZONTAL_AIR_DIRECTION = "horizontal_air_direction"
CONF_SELF_CLEAN = "self_clean"
CONF_TIME_SYNC_INTERVAL = "time_sync_interval"
CONF_ENERGY = "energy"

CONF_ECO = "eco"
CONF_HI_POWER = "hi_power"
CONF_FIREPLACE = "fireplace"
CONF_EIGHT_DEGREE_HEAT = "eight_degree_heat"
CONF_OUTDOOR_SILENT = "outdoor_silent"
CONF_SLEEP = "sleep"
CONF_FLOOR = "floor"
CONF_COMFORT = "comfort"
CONF_PURE = "pure"
CONF_START_DEFROST = "start_defrost"
CONF_STRONG_DEFROST = "strong_defrost"
CONF_DEFROST_ACTIVE = "defrost_active"

CONF_SPECIAL_MODE = "special_mode"
CONF_SPECIAL_MODE_MODES = "modes"
CONF_SUPPORTED_PRESETS = "supported_presets"

FEATURE_HORIZONTAL_SWING = "horizontal_swing"
MIN_TEMP = "min_temp"
DISABLE_HEAT_MODE = "disable_heat_mode"
DISABLE_WIFI_LED = "disable_wifi_led"

toshiba_ns = cg.esphome_ns.namespace("toshiba_suzumi")
ToshibaClimateUart = toshiba_ns.class_("ToshibaValidatedControlUart", cg.PollingComponent, climate.Climate, uart.UARTDevice)
ToshibaPwrModeSelect = toshiba_ns.class_("ToshibaValidatedPowerSelect", select.Select)
ToshibaSpecialModeSelect = toshiba_ns.class_("ToshibaSpecialModeSelect", select.Select)
ToshibaVerticalAirDirectionSelect = toshiba_ns.class_("ToshibaValidatedVerticalAirDirectionSelect", select.Select)
ToshibaHorizontalAirDirectionSelect = toshiba_ns.class_("ToshibaHorizontalAirDirectionSelect", select.Select)
ToshibaValidatedFunctionSwitch = toshiba_ns.class_("ToshibaValidatedFunctionSwitch", switch.Switch)
ToshibaValidatedSilentSelect = toshiba_ns.class_("ToshibaValidatedSilentSelect", select.Select)
ToshibaPureSwitch = toshiba_ns.class_("ToshibaPureSwitch", switch.Switch)
ToshibaDefrostButton = toshiba_ns.class_("ToshibaDefrostButton", button.Button)
ToshibaSpecialModeSwitch = toshiba_ns.class_("ToshibaSpecialModeSwitch", switch.Switch)
ToshibaSpecialModeLevelSelect = toshiba_ns.class_("ToshibaSpecialModeLevelSelect", select.Select)

SPECIAL_MODE_VALUES = {
    CONF_HI_POWER: 1,
    CONF_ECO: 3,
    CONF_EIGHT_DEGREE_HEAT: 4,
    CONF_SLEEP: 5,
    CONF_FLOOR: 6,
    CONF_COMFORT: 7,
}

CONFIG_SCHEMA = climate.climate_schema(ToshibaClimateUart).extend(
    {
        cv.GenerateID(): cv.declare_id(ToshibaClimateUart),
        cv.Optional(CONF_INDOOR_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_OUTDOOR_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_ODU_DISCHARGE_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_ODU_SUCTION_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_ODU_HEAT_EXCHANGER_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_COMPRESSOR_LOAD): sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, accuracy_decimals=1, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_COMPRESSOR_CURRENT): sensor.sensor_schema(unit_of_measurement=UNIT_AMPERE, accuracy_decimals=1, device_class=DEVICE_CLASS_CURRENT, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_IDU_HEAT_EXCHANGER_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_IDU_JUNCTION_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, accuracy_decimals=0, device_class=DEVICE_CLASS_TEMPERATURE, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_IDU_FAN_SPEED): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_REGISTER_90_RAW): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_REGISTER_94_RAW): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_REGISTER_C7_RAW): sensor.sensor_schema(accuracy_decimals=0, state_class=STATE_CLASS_MEASUREMENT),
        cv.Optional(CONF_IDU_MODEL): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_ODU_MODEL): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_PWR_SELECT): select.select_schema(ToshibaPwrModeSelect).extend({cv.GenerateID(): cv.declare_id(ToshibaPwrModeSelect)}),
        cv.Optional(CONF_VERTICAL_AIR_DIRECTION): select.select_schema(ToshibaVerticalAirDirectionSelect).extend({cv.GenerateID(): cv.declare_id(ToshibaVerticalAirDirectionSelect)}),
        cv.Optional(CONF_HORIZONTAL_AIR_DIRECTION): select.select_schema(ToshibaHorizontalAirDirectionSelect).extend({cv.GenerateID(): cv.declare_id(ToshibaHorizontalAirDirectionSelect)}),
        cv.Optional(CONF_SELF_CLEAN): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_RUNNING),
        cv.Optional(CONF_DEFROST_ACTIVE): binary_sensor.binary_sensor_schema(device_class=DEVICE_CLASS_RUNNING),
        cv.Optional(CONF_ECO): switch.switch_schema(ToshibaValidatedFunctionSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaValidatedFunctionSwitch)}),
        cv.Optional(CONF_HI_POWER): switch.switch_schema(ToshibaValidatedFunctionSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaValidatedFunctionSwitch)}),
        cv.Optional(CONF_EIGHT_DEGREE_HEAT): switch.switch_schema(ToshibaValidatedFunctionSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaValidatedFunctionSwitch)}),
        cv.Optional(CONF_OUTDOOR_SILENT): select.select_schema(ToshibaValidatedSilentSelect).extend({cv.GenerateID(): cv.declare_id(ToshibaValidatedSilentSelect)}),
        cv.Optional(CONF_PURE): switch.switch_schema(ToshibaPureSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaPureSwitch)}),
        cv.Optional(CONF_START_DEFROST): button.button_schema(ToshibaDefrostButton).extend({cv.GenerateID(): cv.declare_id(ToshibaDefrostButton)}),
        cv.Optional(CONF_STRONG_DEFROST): button.button_schema(ToshibaDefrostButton).extend({cv.GenerateID(): cv.declare_id(ToshibaDefrostButton)}),
        cv.Optional(CONF_SLEEP): switch.switch_schema(ToshibaSpecialModeSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaSpecialModeSwitch)}),
        cv.Optional(CONF_FLOOR): switch.switch_schema(ToshibaSpecialModeSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaSpecialModeSwitch)}),
        cv.Optional(CONF_COMFORT): switch.switch_schema(ToshibaSpecialModeSwitch).extend({cv.GenerateID(): cv.declare_id(ToshibaSpecialModeSwitch)}),
        cv.Optional(CONF_FIREPLACE): select.select_schema(ToshibaSpecialModeLevelSelect).extend({cv.GenerateID(): cv.declare_id(ToshibaSpecialModeLevelSelect)}),
        cv.Optional(FEATURE_HORIZONTAL_SWING): cv.boolean,
        cv.Optional(DISABLE_WIFI_LED): cv.boolean,
        cv.Optional(DISABLE_HEAT_MODE): cv.boolean,
        cv.Optional(CONF_SPECIAL_MODE): select.select_schema(ToshibaSpecialModeSelect).extend({
            cv.GenerateID(): cv.declare_id(ToshibaSpecialModeSelect),
            cv.Required(CONF_SPECIAL_MODE_MODES): cv.ensure_list(cv.one_of("Standard", "Hi POWER", "ECO", "Fireplace 1", "Fireplace 2", "8 degrees", "Silent#1", "Silent#2", "Sleep", "Floor", "Comfort"))
        }),
        cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(cv.one_of("Standard", "Hi POWER", "ECO", "Fireplace 1", "Fireplace 2", "8 degrees", "Silent#1", "Silent#2", "Sleep", "Floor", "Comfort")),
        cv.Optional(MIN_TEMP): cv.int_,
        cv.Optional(CONF_TIME_ID): cv.use_id(cg.esphome_ns.namespace("time").class_("RealTimeClock")),
        cv.Optional(CONF_TIME_SYNC_INTERVAL, default="24h"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_ENERGY): sensor.sensor_schema(unit_of_measurement=UNIT_WATT_HOURS, accuracy_decimals=0, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING),
    }
).extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema("120s"))

async def _register_special_switch(config, parent, key, mode_value, setter_name):
    if key not in config:
        return
    ent = await switch.new_switch(config[key])
    await cg.register_parented(ent, config[CONF_ID])
    cg.add(ent.set_special_mode(mode_value))
    cg.add(getattr(parent, setter_name)(ent))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    sensor_setters = {
        CONF_INDOOR_TEMP: "set_indoor_temp_sensor", CONF_OUTDOOR_TEMP: "set_outdoor_temp_sensor",
        CONF_ODU_DISCHARGE_TEMP: "set_odu_discharge_temp_sensor", CONF_ODU_SUCTION_TEMP: "set_odu_suction_temp_sensor",
        CONF_ODU_HEAT_EXCHANGER_TEMP: "set_odu_heat_exchanger_temp_sensor", CONF_COMPRESSOR_LOAD: "set_compressor_load_sensor",
        CONF_COMPRESSOR_CURRENT: "set_compressor_current_sensor", CONF_IDU_HEAT_EXCHANGER_TEMP: "set_idu_heat_exchanger_temp_sensor",
        CONF_IDU_JUNCTION_TEMP: "set_idu_junction_temp_sensor", CONF_IDU_FAN_SPEED: "set_idu_fan_speed_sensor",
        CONF_REGISTER_90_RAW: "set_register_90_raw_sensor", CONF_REGISTER_94_RAW: "set_register_94_raw_sensor",
        CONF_REGISTER_C7_RAW: "set_register_c7_raw_sensor", CONF_ENERGY: "set_energy_sensor",
    }
    for key, setter in sensor_setters.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    if CONF_IDU_MODEL in config:
        sens = await text_sensor.new_text_sensor(config[CONF_IDU_MODEL])
        cg.add(var.set_idu_model_sensor(sens))
    if CONF_ODU_MODEL in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ODU_MODEL])
        cg.add(var.set_odu_model_sensor(sens))

    if CONF_PWR_SELECT in config:
        sel = await select.new_select(config[CONF_PWR_SELECT], options=["50 %", "75 %", "100 %"])
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_pwr_select(sel))

    fixed_options = ["Position 1", "Position 2", "Position 3", "Position 4", "Position 5"]
    if CONF_VERTICAL_AIR_DIRECTION in config:
        sel = await select.new_select(config[CONF_VERTICAL_AIR_DIRECTION], options=fixed_options)
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_vertical_air_direction_select(sel))
    if CONF_HORIZONTAL_AIR_DIRECTION in config:
        sel = await select.new_select(config[CONF_HORIZONTAL_AIR_DIRECTION], options=fixed_options)
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_horizontal_air_direction_select(sel))

    if CONF_SELF_CLEAN in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_SELF_CLEAN])
        cg.add(var.set_self_clean_sensor(sens))
    if CONF_DEFROST_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_DEFROST_ACTIVE])
        cg.add(var.set_defrost_active_sensor(sens))

    await _register_special_switch(config, var, CONF_ECO, SPECIAL_MODE_VALUES[CONF_ECO], "set_eco_switch")
    await _register_special_switch(config, var, CONF_HI_POWER, SPECIAL_MODE_VALUES[CONF_HI_POWER], "set_hi_power_switch")
    await _register_special_switch(config, var, CONF_EIGHT_DEGREE_HEAT, SPECIAL_MODE_VALUES[CONF_EIGHT_DEGREE_HEAT], "set_eight_degree_heat_switch")
    await _register_special_switch(config, var, CONF_SLEEP, SPECIAL_MODE_VALUES[CONF_SLEEP], "set_sleep_switch")
    await _register_special_switch(config, var, CONF_FLOOR, SPECIAL_MODE_VALUES[CONF_FLOOR], "set_floor_switch")
    await _register_special_switch(config, var, CONF_COMFORT, SPECIAL_MODE_VALUES[CONF_COMFORT], "set_comfort_switch")

    if CONF_FIREPLACE in config:
        sel = await select.new_select(config[CONF_FIREPLACE], options=["Off", "Fireplace 1", "Fireplace 2"])
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(sel.set_special_modes(32, 48))
        cg.add(sel.set_option_names("Fireplace 1", "Fireplace 2"))
        cg.add(var.set_fireplace_select(sel))
    if CONF_OUTDOOR_SILENT in config:
        sel = await select.new_select(config[CONF_OUTDOOR_SILENT], options=["Standard", "Silent 1", "Silent 2"])
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_outdoor_silent_select(sel))
    if CONF_PURE in config:
        ent = await switch.new_switch(config[CONF_PURE])
        await cg.register_parented(ent, config[CONF_ID])
        cg.add(var.set_pure_switch(ent))
    if CONF_START_DEFROST in config:
        ent = await button.new_button(config[CONF_START_DEFROST])
        await cg.register_parented(ent, config[CONF_ID])
        cg.add(ent.set_strong(False))
    if CONF_STRONG_DEFROST in config:
        ent = await button.new_button(config[CONF_STRONG_DEFROST])
        await cg.register_parented(ent, config[CONF_ID])
        cg.add(ent.set_strong(True))

    if FEATURE_HORIZONTAL_SWING in config:
        cg.add(var.set_horizontal_swing(config[FEATURE_HORIZONTAL_SWING]))
    if MIN_TEMP in config:
        cg.add(var.set_min_temp(config[MIN_TEMP]))
    if DISABLE_HEAT_MODE in config:
        cg.add(var.disable_heat_mode(config[DISABLE_HEAT_MODE]))
    if DISABLE_WIFI_LED in config:
        cg.add(var.disable_wifi_led(config[DISABLE_WIFI_LED]))
    if CONF_EIGHT_DEGREE_HEAT in config:
        cg.add(var.set_min_temp(5))

    if CONF_SUPPORTED_PRESETS in config:
        presets = config[CONF_SUPPORTED_PRESETS]
        cg.add(var.set_supported_presets(presets))
        if "8 degrees" in presets:
            cg.add(var.set_min_temp(5))
    if CONF_SPECIAL_MODE in config:
        presets = config[CONF_SPECIAL_MODE][CONF_SPECIAL_MODE_MODES]
        cg.add(var.set_supported_presets(presets))
        if "8 degrees" in presets:
            cg.add(var.set_min_temp(5))
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(time_))
    if CONF_TIME_SYNC_INTERVAL in config:
        cg.add(var.set_time_sync_interval(config[CONF_TIME_SYNC_INTERVAL]))
