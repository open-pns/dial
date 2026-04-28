#pragma once
#include <stdint.h>
#include "hardware/i2c.h"

class MPU6050 {
public:
    static constexpr float    GYRO_SCALE  = 131.0f;
    static constexpr uint32_t TIMEOUT_US  = 10000;

    MPU6050(i2c_inst_t *port, uint8_t addr);

    bool    init();
    int16_t readAxis(uint8_t reg);
    void    calibrate(float &ox, float &oy, float &oz, int samples = 500);

private:
    i2c_inst_t *_port;
    uint8_t     _addr;
};
