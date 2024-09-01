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

#define ACCEL_CONFIG 0x1C
#define ACCEL_CONFIG_16G 0x03

typedef enum {REST, ACC_FWD, DEC_FWD, ACC_BACK, DEC_BACK}motion_state_t;

motion_state_t x_axis = REST, y_axis = REST; 

int main() {
    int file;
    char *filename = "/dev/i2c-2"; // Change to your I2C bus device
    while ((file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open I2C bus");
        //return 1;
    }

    while (ioctl(file, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to connect to sensor");
        //close(file);
        //return 1;
    }

    // Wake up the MPU6050 (default is sleep mode)
    char buf[2] = { MPU6050_REG_PWR_MGMT_1, 0 };
    while (write(file, buf, 2) != 2) {
        perror("Failed to wake up MPU6050");
        //close(file);
        //return 1;
    }

//    buf[0] = ACCEL_CONFIG;
//    buf[1] = ACCEL_CONFIG_16G;
//    if (write(file, buf, 2) != 2) {
//        perror("Failed to write to the i2c bus.");
//        close(file);
//        return 1;
//    }

//    printf("MPU6050 sensitivity set to ±16g\n");

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
    int16_t vz = 0, x = 750, y = 750, z = 0;
    int16_t dAz = 0;
    float vy, vx;
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
//            close(file);
//            return 1;
        }

        if (read(file, buf, 6) != 6) {
            perror("Failed to read data");
//            close(file);
//            return 1;
        }

        int16_t ax = (((buf[0] << 8) | buf[1]) - init_ax);
        if (abs(ax) <= 300)
        {
            x_axis = REST;
            ax = 0;
            vx = 0;
        }
        else if (ax < 0)
        {
            if (x_axis == REST)
            {
                x_axis = ACC_BACK;
                vx -= ((float)ax * -0.01);
            }
            else if (x_axis == ACC_FWD)
            {
                x_axis = DEC_FWD;
                vx += ((float)ax * -0.01);
            }
        }
        else if (ax > 0)
        {
            if (x_axis == REST)
            {
                x_axis = ACC_FWD;
                vx += ((float)ax * 0.01);
            }
            else if (x_axis == ACC_BACK)
            {
                x_axis = DEC_BACK;
                vx -= ((float)ax * 0.01);
            }
        }

        x += vx;

        int16_t ay = (((buf[2] << 8) | buf[3]) - init_ay);
        if (abs(ay) <= 300)
        {
            y_axis = REST;
            ay = 0;
            vy = 0;
        }
        else if (ay < 0)
        {
            if (y_axis == REST)
            {
                y_axis = ACC_BACK;
                vy -= ((float)ay * -0.01);
            }
            else if (y_axis == ACC_FWD)
            {
                y_axis = DEC_FWD;
                vy += ((float)ay * -0.01);
            }
        }
        else if (ay > 0)
        {
            if (y_axis == REST)
            {
                y_axis = ACC_FWD;
                vy += ((float)ay * 0.01);
            }
            else if (y_axis == ACC_BACK)
            {
                y_axis = DEC_BACK;
                vy -= ((float)ay * 0.01);
            }
        }

        y += vy;
        int16_t az = ((buf[4] << 8) | buf[5]);
        //if (abs(az) <= 100)
        //{
        //    az = 0;
        //}
        vz += az * 0.01;
        z += vz;
        
//        printf("Accelerometer data:\n");
        printf("AX: %d, VX = %f, X = %d, state = %d\n", ax, vx, x, x_axis);
//        printf("AY: %d, VY = %d, Y = %d\n", ay, vy, y);
//        printf("AZ: %d, VZ = %d, Z = %d\n", az, vz, z);
       printf("AY: %d, VY = %f, Y = %d, state = %d\n", ay, vy, y, y_axis); 
    }
    


    close(file);
    return 0;
}