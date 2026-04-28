#include "../inc/MPU6050.h"
#include "../inc/mpu6050_registers.h"
#include <stdio.h>
#include "pico/stdlib.h"

MPU6050::MPU6050(i2c_inst_t *port, uint8_t addr)
    : _port(port), _addr(addr) {}

bool MPU6050::init() {
    uint8_t who_reg = REG_WHO_AM_I;
    uint8_t who_val = 0;
    if (i2c_write_timeout_us(_port, _addr, &who_reg, 1, true, TIMEOUT_US) < 0 ||
        i2c_read_timeout_us(_port, _addr, &who_val, 1, false, TIMEOUT_US) < 0) {
        printf("ERROR: MPU6050 not responding on I2C (check wiring/power)\n");
        return false;
    }
    if (who_val != 0x68) {
        printf("ERROR: Unexpected WHO_AM_I: 0x%02X (expected 0x68)\n", who_val);
        return false;
    }

    uint8_t wake[] = {REG_PWR_MGMT_1, 0x00};
    if (i2c_write_timeout_us(_port, _addr, wake, 2, false, TIMEOUT_US) < 0) {
        printf("ERROR: Failed to wake MPU6050\n");
        return false;
    }

    uint8_t range[] = {REG_GYRO_CONFIG, 0x00};
    if (i2c_write_timeout_us(_port, _addr, range, 2, false, TIMEOUT_US) < 0) {
        printf("ERROR: Failed to configure gyro range\n");
        return false;
    }

    return true;
}

int16_t MPU6050::readAxis(uint8_t reg) {
    uint8_t buf[2];
    if (i2c_write_timeout_us(_port, _addr, &reg, 1, true, TIMEOUT_US) < 0 ||
        i2c_read_timeout_us(_port, _addr, buf, 2, false, TIMEOUT_US) < 0) {
        printf("\nERROR: I2C read failed for reg 0x%02X\n", reg);
        return 0;
    }
    return (int16_t)((buf[0] << 8) | buf[1]);
}

void MPU6050::calibrate(float &ox, float &oy, float &oz, int samples) {
    printf("\nCalibrating — hold sensor still...\n");
    float sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < samples; i++) {
        sx += readAxis(REG_GYRO_XOUT_H);
        sy += readAxis(REG_GYRO_XOUT_H + 2);
        sz += readAxis(REG_GYRO_XOUT_H + 4);
        sleep_ms(2);
    }
    ox = (sx / samples) / GYRO_SCALE;
    oy = (sy / samples) / GYRO_SCALE;
    oz = (sz / samples) / GYRO_SCALE;
    printf("Calibration done. Offsets — X: %.3f  Y: %.3f  Z: %.3f (deg/s)\n", ox, oy, oz);
}
