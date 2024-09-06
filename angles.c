#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>

#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_YOUT_L 0x3E
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_ACCEL_ZOUT_L 0x40
#define MPU6050_GYRO_XOUT_H 0x43
#define MPU6050_GYRO_XOUT_L 0x44
#define MPU6050_GYRO_YOUT_H 0x45
#define MPU6050_GYRO_YOUT_L 0x46
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_GYRO_ZOUT_L 0x48

// Function to initialize MPU6050
void MPU6050_Init(int file) {
    // Wake up the MPU6050
    uint8_t buf[2];
    buf[0] = MPU6050_PWR_MGMT_1;
    buf[1] = 0x00; // Wake up the MPU6050
    if (write(file, buf, 2) != 2) {
        perror("Failed to wake up MPU6050");
        exit(1);
    }
}

// Function to read raw data from MPU6050
int16_t MPU6050_Read_16(int file, uint8_t reg) {
    uint8_t buf[2];
    if (write(file, &reg, 1) != 1) {
        perror("Failed to set register");
        exit(1);
    }
    if (read(file, buf, 2) != 2) {
        perror("Failed to read data");
        exit(1);
    }
    return (int16_t)((buf[0] << 8) | buf[1]);
}

// Function to calculate Euler angles
void Calculate_Euler_Angles(float ax, float ay, float az, float gx, float gy, float gz, float *roll, float *pitch, float *yaw) {
    // Convert raw accelerometer data to angles
    *roll = atan2(ay, az) * 180.0 / M_PI;
    *pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;

    // Simple integration for yaw (this is a rough estimate)
    static float prev_gz = 0;
    static float delta_t = 0.01; // Time interval in seconds (update this based on your setup)

    *yaw += gz * delta_t; // Update yaw using gyro data
    if (*yaw > 360.0) *yaw -= 360.0;
    //if (*yaw < 0.0) *yaw += 360.0;

    prev_gz = gz;
}

int main() {
    const char *i2c_dev = "/dev/i2c-2"; // Adjust based on your setup
    int file = open(i2c_dev, O_RDWR);
    if (file < 0) {
        perror("Failed to open the I2C bus");
        exit(1);
    }

    if (ioctl(file, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to MPU6050");
        exit(1);
    }

    MPU6050_Init(file);

    while (1) {
        int16_t ax = MPU6050_Read_16(file, MPU6050_ACCEL_XOUT_H);
        int16_t ay = MPU6050_Read_16(file, MPU6050_ACCEL_YOUT_H);
        int16_t az = MPU6050_Read_16(file, MPU6050_ACCEL_ZOUT_H);
        int16_t gx = MPU6050_Read_16(file, MPU6050_GYRO_XOUT_H);
        int16_t gy = MPU6050_Read_16(file, MPU6050_GYRO_YOUT_H);
        int16_t gz = MPU6050_Read_16(file, MPU6050_GYRO_ZOUT_H);

        // Convert raw data to proper units (e.g., g for accelerometer and degrees/s for gyroscope)
        float ax_f = ax / 16384.0;
        float ay_f = ay / 16384.0;
        float az_f = az / 16384.0;
        float gx_f = gx / 131.0;
        float gy_f = gy / 131.0;
        float gz_f = gz / 131.0;

        float roll, pitch, yaw;
        Calculate_Euler_Angles(ax_f, ay_f, az_f, gx_f, gy_f, gz_f, &roll, &pitch, &yaw);

        printf("Roll: %.2f, Pitch: %.2f, Yaw: %.2f\n", roll, pitch, yaw);

        usleep(50000); // Sleep for 50ms
    }

    close(file);
    return 0;
}