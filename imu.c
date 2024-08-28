#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>

// MPU6050 I2C Address
#define MPU6050_ADDR 0x68

// MPU6050 Register Addresses
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_ACCEL_XOUT_L 0x3C
#define MPU6050_REG_ACCEL_YOUT_H 0x3D
#define MPU6050_REG_ACCEL_YOUT_L 0x3E
#define MPU6050_REG_ACCEL_ZOUT_H 0x3F
#define MPU6050_REG_ACCEL_ZOUT_L 0x40

int main() {
    int file;
    char *filename = "/dev/i2c-2"; // Change to your I2C bus device
    if ((file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to connect to sensor");
        close(file);
        return 1;
    }

    // Wake up the MPU6050 (default is sleep mode)
    char buf[2] = { MPU6050_REG_PWR_MGMT_1, 0 };
    if (write(file, buf, 2) != 2) {
        perror("Failed to wake up MPU6050");
        close(file);
        return 1;
    }

    // Read accelerometer data
    buf[0] = MPU6050_REG_ACCEL_XOUT_H;
    if (write(file, buf, 1) != 1) {
        perror("Failed to set register address");
        close(file);
        return 1;
    }

    if (read(file, buf, 6) != 6) {
        perror("Failed to read data");
        close(file);
        return 1;
    }

    int16_t ax = (buf[0] << 8) | buf[1];
    int16_t ay = (buf[2] << 8) | buf[3];
    int16_t az = (buf[4] << 8) | buf[5];

    printf("Accelerometer data:\n");
    printf("X: %d\n", ax);
    printf("Y: %d\n", ay);
    printf("Z: %d\n", az);

    close(file);
    return 0;
}
