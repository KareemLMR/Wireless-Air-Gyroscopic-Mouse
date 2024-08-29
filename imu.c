#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

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

    int16_t init_ax = (buf[0] << 8) | buf[1];
    int16_t init_ay = (buf[2] << 8) | buf[3];
    int16_t init_az = (buf[4] << 8) | buf[5];

    printf("Accelerometer data:\n");
    printf("X: %d\n", init_ax);
    printf("Y: %d\n", init_ay);
    printf("Z: %d\n", init_az); 
    // Read accelerometer data
    int16_t vx = 0, vy = 0, vz = 0, x = 0, y = 0, z = 0;
    while (1)
    {
        struct timespec req;
        struct timespec rem;

        // Set the sleep duration
        req.tv_sec = 0;  // seconds
        req.tv_nsec = 100000000;  // nanoseconds (1 milli seconds)

        printf("Sleeping for %ld seconds and %ld nanoseconds...\n", req.tv_sec, req.tv_nsec);

        // Sleep for the specified duration
        if (nanosleep(&req, &rem) == -1) {
            perror("nanosleep");
            return 1;
        }

        printf("Awoke after sleeping.\n");
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

        int16_t ax = ((buf[0] << 8) | buf[1]) - init_ax;
        if (abs(ax) <= 100)
        {
            ax = 0;
        }
        vx += ax * 0.001;
        x += vx;
        int16_t ay = ((buf[2] << 8) | buf[3]) - init_ay;
        if (abs(ay) <= 200)
        {
            ay = 0;
        }
        vy += ay * 0.001;
        y += vy;
        int16_t az = ((buf[4] << 8) | buf[5]) - init_az;
        //if (abs(az) <= 100)
        //{
        //    az = 0;
        //}
        vz += az * 0.001;
        z += vz;
        printf("Accelerometer data:\n");
        printf("AX: %d, VX = %d, X = %d\n", ax, vx, x);
        printf("AY: %d, VY = %d, Y = %d\n", ay, vy, y);
        printf("AZ: %d, VZ = %d, Z = %d\n", az, vz, z); 
    }
    


    close(file);
    return 0;
}