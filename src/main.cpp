//NOTE: a good chunk of this code is re used from a project I have been
//working on as a side project. This was originally all C - so I wrapped
//some things in classes for this project. All java code is entirely fresh.

//My foundations come from ece362. I understand that this is pretty out of scope
//for this assignment.

#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "../inc/MPU6050.h"
#include "../inc/mpu6050_registers.h"

// I2C
#define I2C_PORT    i2c0
#define SDA_PIN     4
#define SCL_PIN     5

// ADC
#define ADC_PIN             40
#define ADC_CHANNEL         0
#define ADC1_PIN            41
#define ADC1_CHANNEL        1
#define ADC_VREF            3.3f
#define ADC_RESOLUTION      4095.0f

// Calibration button
#define CAL_BUTTON_PIN      21
#define CAL_SAMPLES         500

// Dial
#define DEGREES_PER_TICK    3.6f   // 360° sweeps full 0-100 range

struct Dial {
    int   value      = 0;
    float angleAccum = 0.0f;

    void update(float gyroX, float voltage, float threshold, float dt) {
        if (voltage <= threshold) {
            angleAccum = 0.0f;
            return;
        }

        angleAccum += gyroX * dt;
        int ticks = (int)(angleAccum / DEGREES_PER_TICK);
        if (ticks != 0) {
            value      += ticks;
            angleAccum -= ticks * DEGREES_PER_TICK;
        }

        if (value > 100) value = 100;
        if (value < 0)   value = 0;
    }
};

int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_gpio_init(ADC1_PIN);

    gpio_init(CAL_BUTTON_PIN);
    gpio_set_dir(CAL_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(CAL_BUTTON_PIN);

    sleep_ms(2000);
    printf("PNS Virtual Dial Starting...\n");

    MPU6050 imu(I2C_PORT, MPU6050_ADDR);
    if (!imu.init()) {
        printf("FATAL: IMU init failed. Halting.\n");
        while (true) tight_loop_contents();
    }

    float ox = 0, oy = 0, oz = 0;
    Dial  dial;
    absolute_time_t loopTime = get_absolute_time();

    while (true) {
        absolute_time_t now = get_absolute_time();
        float dt = absolute_time_diff_us(loopTime, now) * 1e-6f;
        loopTime = now;

        int16_t raw_x = imu.readAxis(REG_GYRO_XOUT_H);
        int16_t raw_y = imu.readAxis(REG_GYRO_XOUT_H + 2);
        int16_t raw_z = imu.readAxis(REG_GYRO_XOUT_H + 4);

        if (!gpio_get(CAL_BUTTON_PIN)) {
            sleep_ms(20);
            if (!gpio_get(CAL_BUTTON_PIN)) {
                imu.calibrate(ox, oy, oz, CAL_SAMPLES);
                while (!gpio_get(CAL_BUTTON_PIN)) tight_loop_contents();
            }
        }

        float gx = (raw_x / MPU6050::GYRO_SCALE) - ox;
        float gy = (raw_y / MPU6050::GYRO_SCALE) - oy;
        float gz = (raw_z / MPU6050::GYRO_SCALE) - oz;

        if (gx > -0.5f && gx < 0.5f) gx = 0.0f;
        if (gy > -0.5f && gy < 0.5f) gy = 0.0f;
        if (gz > -0.5f && gz < 0.5f) gz = 0.0f;

        adc_select_input(ADC_CHANNEL);
        float voltage   = (adc_read() / ADC_RESOLUTION) * ADC_VREF;

        adc_select_input(ADC1_CHANNEL);
        float threshold = (adc_read() / ADC_RESOLUTION) * ADC_VREF;

        dial.update(-gy, voltage, threshold, dt);

        printf("%.2f,%.2f,%.2f,%.3f,%.3f,%d\n", gx, gy, gz, voltage, threshold, dial.value);
        fflush(stdout);

        sleep_ms(5);
    }

    return 0;
}
