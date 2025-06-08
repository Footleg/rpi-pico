/*
 * LSM6DS3 accelerometer driver class ported from the Pimoroni python class.
 *
 * MIT License
 *
 * Copyright (c) 2024 Pimoroni Ltd. <- original Python driver
 * Copyright (c) 2025 Dr Footleg    <- C++ port
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "lsm6ds3.hpp"

// using namespace pimoroni;

LSM6DS3::LSM6DS3(I2C *i2c, uint8_t addr, uint8_t mode)
    : i2c_bus(i2c), address(addr), mode(mode) {
  // Set gyro mode/enable
  writeRegister(CTRL2_G, mode);

  // Set accel mode/enable
  writeRegister(CTRL1_XL, mode);

  // Send the reset bit to clear the pedometer step count
  writeRegister(CTRL10_C, RESET_STEPS);

  // Enable sensor functions (Tap, Tilt, Significant Motion)
  writeRegister(CTRL10_C, SET_FUNC_EN);

  // Enable X Y Z Tap Detection
  writeRegister(TAP_CFG, TAP_EN_XYZ);

  // Enable Double tap
  writeRegister(WAKE_UP_THS, DOUBLE_TAP_EN);

  // Set tap threshold
  writeRegister(TAP_THS_6D, TAP_THRESHOLD);

  // Set double tap max time gap
  writeRegister(INT_DUR2, DOUBLE_TAP_DUR);
}

void LSM6DS3::writeRegister(uint8_t reg, uint8_t value) {
  // uint8_t data[] = {reg, value};
  // i2c_bus->write(address, data, 2);
  i2c_bus->reg_write_uint8(address, reg, value);
}

uint8_t LSM6DS3::readRegister(uint8_t reg) {
  // i2c_bus->write(address, &reg, 1);
  // uint8_t result;
  // i2c_bus->read(address, &result, 1);
  uint8_t result = i2c_bus->reg_read_uint8(address, reg);
  return result;
}

int16_t LSM6DS3::twosComp(int16_t val) {
  if (val & (1 << 15)) {  // If the 16-bit value is negative (based on MSB)
    val -= (1 << 16);     // Convert to signed value
  }
  return val;
}

LSM6DS3::SensorData LSM6DS3::getReadings() {
  uint8_t data[12];  // Buffer to store the raw sensor data
  i2c_bus->read_bytes(address, OUTX_L_G, data,
                      12);  // Read 12 bytes starting from OUTX_L_G

  SensorData readings;

  // Convert raw data to signed integers
  readings.gx = (data[1] << 8) | data[0];
  readings.gx = twosComp(readings.gx);

  readings.gy = (data[3] << 8) | data[2];
  readings.gy = twosComp(readings.gy);

  readings.gz = (data[5] << 8) | data[4];
  readings.gz = twosComp(readings.gz);

  readings.ax = (data[7] << 8) | data[6];
  readings.ax = twosComp(readings.ax);

  readings.ay = (data[9] << 8) | data[8];
  readings.ay = twosComp(readings.ay);

  readings.az = (data[11] << 8) | data[10];
  readings.az = twosComp(readings.az);

  return readings;
}

bool LSM6DS3::singleTapDetected() {
  return (readRegister(TAP_SRC) >> 5) & 0x01;
}
