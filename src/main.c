#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "../inc/mpu6050_registers.h"

// --- Constants ---
#define I2C_PORT        i2c0
#define SDA_PIN         4
#define SCL_PIN         5
#define MPU6050_ADDR    0x68
#define I2C_TIMEOUT_US  10000  // 10ms timeout per I2C transaction

#define CAL_BUTTON_PIN      21
#define CAL_SAMPLES         500  // ~1 second of samples at 2ms each

#define ADC_PIN             40
#define ADC_CHANNEL         0
#define ADC_VREF            3.3f
#define ADC_RESOLUTION      4095.0f  // 12-bit
#define ADC_GATE_THRESHOLD  1.0f     // volts
#define GATE_HOLD_MS        20       // initial debounce before first tick (ms)
#define DIAL_REPEAT_MS      600      // interval between subsequent ticks while held (ms)

// Registers
#define REG_PWR_MGMT_1  0x6B
#define REG_WHO_AM_I    0x75
#define REG_GYRO_XOUT_H 0x43

// Sensitivity Scale Factor
// For range +/- 250 degrees/sec (default), scale is 131.0
#define GYRO_SCALE      131.0

/*
void mpu6050_init(){
    uint8_t data[] = {REG_PWR_MGMT_1, 0x00};
    i2c_write_blocking(i2c_default, MPU6050_ADDR, data, 2, false); //wake chip
}
*/

bool mpu6050_init() {
    // Verify chip identity before configuring
    uint8_t who_reg = REG_WHO_AM_I;
    uint8_t who_val = 0;
    if (i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &who_reg, 1, true, I2C_TIMEOUT_US) < 0 ||
        i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, &who_val, 1, false, I2C_TIMEOUT_US) < 0) {
        printf("ERROR: MPU6050 not responding on I2C (check wiring/power)\n");
        return false;
    }
    if (who_val != 0x68) {
        printf("ERROR: Unexpected WHO_AM_I: 0x%02X (expected 0x68)\n", who_val);
        return false;
    }

    // Wake up (Write 0 to PWR_MGMT_1)
    uint8_t buf[] = {REG_PWR_MGMT_1, 0x00};
    if (i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, buf, 2, false, I2C_TIMEOUT_US) < 0) {
        printf("ERROR: Failed to wake MPU6050\n");
        return false;
    }

    // Set Gyro Range to +/- 250 dps (most sensitive, good for hand gestures)
    uint8_t conf[] = {REG_GYRO_CONFIG, 0x00};
    if (i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, conf, 2, false, I2C_TIMEOUT_US) < 0) {
        printf("ERROR: Failed to configure gyro range\n");
        return false;
    }

    return true;
}

// Reads two bytes (High + Low) and combines them into a signed 16-bit int.
// Returns 0 and prints an error on I2C failure.
int16_t mpu6050_read_axis(uint8_t reg) {
    uint8_t buffer[2];

    if (i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &reg, 1, true, I2C_TIMEOUT_US) < 0 ||
        i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, buffer, 2, false, I2C_TIMEOUT_US) < 0) {
        printf("\nERROR: I2C read failed for reg 0x%02X\n", reg);
        return 0;
    }

    return (int16_t)((buffer[0] << 8) | buffer[1]);
}

void calibrate_gyro(float *offset_x, float *offset_y, float *offset_z) {
    printf("\nCalibrating — hold sensor still...\n");
    float sum_x = 0, sum_y = 0, sum_z = 0;
    for (int i = 0; i < CAL_SAMPLES; i++) {
        sum_x += mpu6050_read_axis(REG_GYRO_XOUT_H);
        sum_y += mpu6050_read_axis(REG_GYRO_XOUT_H + 2);
        sum_z += mpu6050_read_axis(REG_GYRO_XOUT_H + 4);
        sleep_ms(2);
    }
    *offset_x = (sum_x / CAL_SAMPLES) / GYRO_SCALE;
    *offset_y = (sum_y / CAL_SAMPLES) / GYRO_SCALE;
    *offset_z = (sum_z / CAL_SAMPLES) / GYRO_SCALE;
    printf("Calibration done. Offsets — X: %.3f  Y: %.3f  Z: %.3f (deg/s)\n",
           *offset_x, *offset_y, *offset_z);
}

int main() {
    stdio_init_all();
    
    // Init I2C at 400kHz (Standard Speed)
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Init ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    // Init calibration button
    gpio_init(CAL_BUTTON_PIN);
    gpio_set_dir(CAL_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(CAL_BUTTON_PIN);

    sleep_ms(2000); // Wait for Serial Monitor
    printf("MPU6050 Gyroscope Demo Starting...\n");

    if (!mpu6050_init()) {
        printf("FATAL: IMU init failed. Halting.\n");
        while (1) tight_loop_contents();
    }

    float offset_x = 0, offset_y = 0, offset_z = 0;

    int dial_value = 0;
    absolute_time_t gate_open_since = nil_time;
    bool gate_triggered = false;

    while (1) {
        int16_t raw_x = mpu6050_read_axis(REG_GYRO_XOUT_H);
        int16_t raw_y = mpu6050_read_axis(REG_GYRO_XOUT_H + 2);
        int16_t raw_z = mpu6050_read_axis(REG_GYRO_XOUT_H + 4);

        // Check for calibration button press
        if (!gpio_get(CAL_BUTTON_PIN)) {
            sleep_ms(20); // debounce
            if (!gpio_get(CAL_BUTTON_PIN)) {
                calibrate_gyro(&offset_x, &offset_y, &offset_z);
                while (!gpio_get(CAL_BUTTON_PIN)) tight_loop_contents(); // wait for release
            }
        }

        float gyro_x = (raw_x / GYRO_SCALE) - offset_x;
        float gyro_y = (raw_y / GYRO_SCALE) - offset_y;
        float gyro_z = (raw_z / GYRO_SCALE) - offset_z;

        if (gyro_x < 0.5f && gyro_x > -0.5f) gyro_x = 0.0f;
        if (gyro_y < 0.5f && gyro_y > -0.5f) gyro_y = 0.0f;
        if (gyro_z < 0.5f && gyro_z > -0.5f) gyro_z = 0.0f;

        float voltage = (adc_read() / ADC_RESOLUTION) * ADC_VREF;

        if (voltage > ADC_GATE_THRESHOLD) {
            if (is_nil_time(gate_open_since)) {
                // Signal just crossed threshold — start the clock
                gate_open_since = get_absolute_time();
                gate_triggered = false;
            } else if (!gate_triggered &&
                       absolute_time_diff_us(gate_open_since, get_absolute_time()) >= GATE_HOLD_MS * 1000) {
                // Held above threshold for 20ms — act once, then require a reset
                dial_value += (int)gyro_x;
                if (dial_value > 100) dial_value = 100;
                if (dial_value < 0)   dial_value = 0;
                gate_triggered = true;
            }
        } else {
            // Signal dropped — reset so the full 20ms is required again
            gate_open_since = nil_time;
            gate_triggered = false;
        }

        printf("Rot X: %8.2f | Rot Y: %8.2f | Rot Z: %8.2f (deg/s) | ADC: %5.3fV | Dial: %8d\r",
               gyro_x, gyro_y, gyro_z, voltage, dial_value);
        fflush(stdout);

        sleep_ms(5);
    }
    return 0;
}