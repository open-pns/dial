#ifndef MPU6050_REGISTERS_H
#define MPU6050_REGISTERS_H

// Physical Address
#define MPU6050_ADDR        0x68

// Configuration Registers
#define REG_SMPLRT_DIV      0x19 // Sample Rate Divider
#define REG_CONFIG          0x1A // DLPF (Low Pass Filter) Config
#define REG_GYRO_CONFIG     0x1B // Range (250/500/1000/2000 dps)
#define REG_ACCEL_CONFIG    0x1C // Range (2g/4g/8g/16g)

// Interrupt Control
#define REG_INT_PIN_CFG     0x37 // Interrupt Pin Configuration
#define REG_INT_ENABLE      0x38 // Interrupt Enable

// Power Management
#define REG_PWR_MGMT_1      0x6B // Sleep / Reset / Clock Select
#define REG_WHO_AM_I        0x75 // Should return 0x68

// Data Registers
#define REG_ACCEL_XOUT_H    0x3B
#define REG_GYRO_XOUT_H     0x43

// Size of the data block
#define MPU6050_DATA_LEN    14 

#endif