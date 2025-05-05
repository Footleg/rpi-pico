/*
 * Copyright © 2020 Wolfgang Christl, 
 * © 2025 Dr Footleg (Replaced dependency on LVGL with Pimoroni pico graphics)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the “Software”), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
*/

#include "ft6x36.h"
#include "cstdio"
#include "pimoroni_i2c.hpp"

using namespace pimoroni;

static uint8_t current_dev_addr;       // set during init
static ft6x36_touch_t touch_inputs = { -1, -1, TOUCH_STATE_RELEASED };    // -1 coordinates to designate it was never touched

void FT6X36::ft6x06_i2c_read8(uint8_t slave_addr, uint8_t register_addr, uint8_t *data_buf) {
    *data_buf = i2c->reg_read_uint8(current_dev_addr, register_addr);
}

/**
  * @brief  Read the FT6x36 gesture ID. Initialize first!
  * @param  dev_addr: I2C FT6x36 Slave address.
  * @retval The gesture ID or 0x00 in case of failure
  */
uint8_t FT6X36::ft6x36_get_gesture_id()
{
    uint8_t data_buf;
    int ret;
    ft6x06_i2c_read8(current_dev_addr, FT6X36_GEST_ID_REG, &data_buf);

    return data_buf;
}

/**
  * @brief  Initialize for FT6x36 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of FT6X36).
  * @retval None
  */
FT6X36::FT6X36(I2C *i2c, uint16_t dev_addr)
{
    this->i2c = i2c;
    current_dev_addr = dev_addr;
    uint8_t data_buf;
    int ret;
    ft6x06_i2c_read8(dev_addr, FT6X36_PANEL_ID_REG, &data_buf);

    printf("Device ID: 0x%02x\r\n", data_buf);

    ft6x06_i2c_read8(dev_addr, FT6X36_CHIPSELECT_REG, &data_buf);
    printf("Chip ID: 0x%02x\r\n", data_buf);

    ft6x06_i2c_read8(dev_addr, FT6X36_DEV_MODE_REG, &data_buf);
    printf("Device mode: 0x%02x\r\n", data_buf);

    ft6x06_i2c_read8(dev_addr, FT6X36_FIRMWARE_ID_REG, &data_buf);
    printf("Firmware ID: 0x%02x\r\n", data_buf);

    ft6x06_i2c_read8(dev_addr, FT6X36_RELEASECODE_REG, &data_buf);
    printf("Release code: 0x%02x\r\n", data_buf);
}

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  data: Store data here
  * @retval Always false
  */
bool FT6X36::ft6x36_read(touch_data_t *data) {
    uint8_t data_buf[5];        // 1 byte status, 2 bytes X, 2 bytes Y

    i2c->read_bytes(current_dev_addr, FT6X36_TD_STAT_REG, &data_buf[0], 5);

    uint8_t touch_pnt_cnt = data_buf[0];  // Number of detected touch points

    if (touch_pnt_cnt != 1) {    // ignore no touch & multi touch
        if ( touch_inputs.current_state != TOUCH_STATE_RELEASED)
        {
            touch_inputs.current_state = TOUCH_STATE_RELEASED;
        } 
        data->point.x = touch_inputs.last_x;
        data->point.y = touch_inputs.last_y;
        data->state = touch_inputs.current_state;
        return false;
    }

    touch_inputs.current_state = TOUCH_STATE_PRESSED;
    touch_inputs.last_x = (((data_buf[1] & FT6X36_MSB_MASK) << 8) | (data_buf[2] & FT6X36_LSB_MASK));
    touch_inputs.last_y = (((data_buf[3] & FT6X36_MSB_MASK) << 8) | (data_buf[4] & FT6X36_LSB_MASK));

#if CONFIG_LV_FT6X36_SWAPXY
    int16_t swap_buf = touch_inputs.last_x;
    touch_inputs.last_x = touch_inputs.last_y;
    touch_inputs.last_y = swap_buf;
#endif
#if CONFIG_LV_FT6X36_INVERT_X
    touch_inputs.last_x =  LV_HOR_RES - touch_inputs.last_x;
#endif
#if CONFIG_LV_FT6X36_INVERT_Y
    touch_inputs.last_y = LV_VER_RES - touch_inputs.last_y;
#endif
    data->point.x = touch_inputs.last_x;
    data->point.y = touch_inputs.last_y;
    data->state = touch_inputs.current_state;
    //printf("X=%u Y=%u\r\n", data->point.x, data->point.y);

    return false;
}
