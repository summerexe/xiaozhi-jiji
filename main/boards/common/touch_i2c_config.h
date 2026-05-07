#ifndef TOUCH_I2C_CONFIG_H
#define TOUCH_I2C_CONFIG_H

#include <esp_lcd_io_i2c.h>

#include <cstdint>

inline esp_lcd_panel_io_i2c_config_t MakeTouchI2cConfig(
    uint32_t dev_addr, int lcd_cmd_bits, uint32_t scl_speed_hz = 100000) {
    esp_lcd_panel_io_i2c_config_t config = {};
    config.dev_addr = dev_addr;
    config.control_phase_bytes = 1;
    config.dc_bit_offset = 0;
    config.lcd_cmd_bits = lcd_cmd_bits;
    config.lcd_param_bits = 0;
    config.flags.dc_low_on_data = 0;
    config.flags.disable_control_phase = 1;
    config.scl_speed_hz = scl_speed_hz;
    return config;
}

#endif // TOUCH_I2C_CONFIG_H
