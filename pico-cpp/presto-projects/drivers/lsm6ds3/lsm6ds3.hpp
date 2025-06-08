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
#include "pico/stdlib.h"
#include "pimoroni_i2c.hpp"

using namespace pimoroni;

class LSM6DS3 {
 private:
  // Registers
  static constexpr uint8_t WHO_AM_I = 0x0F;
  static constexpr uint8_t DEFAULT_ADDRESS = 0x6A;
  static constexpr uint8_t CTRL2_G = 0x11;
  static constexpr uint8_t CTRL1_XL = 0x10;
  static constexpr uint8_t CTRL10_C = 0x19;
  static constexpr uint8_t CTRL3_C = 0x12;

  // This is the start of the data registers for the Gyro and Accelerometer
  // There are 12 Bytes in total starting at 0x23 and ending at 0x2D
  static constexpr uint8_t OUTX_L_G = 0x22;

  static constexpr uint8_t STEP_COUNTER_L = 0x4B;
  static constexpr uint8_t STEP_COUNTER_H = 0x4C;
  static constexpr uint8_t TAP_SRC = 0x1C;
  static constexpr uint8_t TAP_CFG = 0x58;
  static constexpr uint8_t FUNC_SRC1 = 0x53;
  static constexpr uint8_t FUNC_SRC2 = 0x54;
  static constexpr uint8_t TAP_THS_6D = 0x59;
  static constexpr uint8_t FREE_FALL = 0x5D;
  static constexpr uint8_t WAKE_UP_THS = 0x5B;
  static constexpr uint8_t WAKE_UP_SRC = 0x1B;
  static constexpr uint8_t INT_DUR2 = 0x5A;

  // CONFIG DATA
  static constexpr uint8_t NORMAL_MODE_104HZ = 0x40;
  static constexpr uint8_t NORMAL_MODE_208HZ = 0x50;
  static constexpr uint8_t PERFORMANCE_MODE_416HZ = 0x60;
  static constexpr uint8_t LOW_POWER_26HZ = 0x02;
  static constexpr uint8_t SET_FUNC_EN = 0xBD;
  static constexpr uint8_t RESET_STEPS = 0x02;
  static constexpr uint8_t TAP_EN_XYZ = 0x8E;
  static constexpr uint8_t TAP_THRESHOLD = 0x02;
  static constexpr uint8_t DOUBLE_TAP_EN = 0x80;
  static constexpr uint8_t DOUBLE_TAP_DUR = 0x20;

  I2C *i2c_bus;
  uint8_t address;
  uint8_t mode;

  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
  int16_t twosComp(int16_t val);

 public:
  LSM6DS3(I2C *i2c, uint8_t addr = DEFAULT_ADDRESS,
          uint8_t mode = NORMAL_MODE_104HZ);

  struct SensorData {
    int16_t ax, ay, az;  // Accelerometer data
    int16_t gx, gy, gz;  // Gyroscope data
  };

  SensorData getReadings();
  bool singleTapDetected();
};
