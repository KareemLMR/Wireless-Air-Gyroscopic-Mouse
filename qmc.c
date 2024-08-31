#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>

#define QMC5883L_ADDR 0x0D // Replace with the actual address if different

// QMC5883L Registers
#define OUT_X_L 0x00
#define OUT_X_H 0x01
#define OUT_Y_L 0x02
#define OUT_Y_H 0x03
#define OUT_Z_L 0x04
#define OUT_Z_H 0x05
#define CTRL_REG1 0x09
#define CTRL_REG2 0x0A
#define STATUS_REG 0x0B

// Define modes, ODR, RNG, OSR
#define Mode_Continuous 0x01
#define ODR_200Hz 0x00
#define RNG_8G 0x00
#define OSR_512 0x00

typedef struct {
    int file;
    uint8_t address;
} MechaQMC5883;

void init_qmc5883l(MechaQMC5883 *sensor) {
    unsigned char config[2];
    config[0] = CTRL_REG1;
    config[1] = 0x01; // Enable continuous measurement
    if (write(sensor->file, config, 2) != 2) {
        perror("Failed to write to I2C bus.");
    }

    // Set mode, ODR, RNG, OSR
    config[0] = CTRL_REG1;
    config[1] = Mode_Continuous | ODR_200Hz | RNG_8G | OSR_512;
    if (write(sensor->file, config, 2) != 2) {
        perror("Failed to write to I2C bus.");
    }
}

void set_mode(MechaQMC5883 *sensor, uint16_t mode, uint16_t odr, uint16_t rng, uint16_t osr) {
    unsigned char config[2];
    config[0] = CTRL_REG1;
    config[1] = mode | odr | rng | osr;
    if (write(sensor->file, config, 2) != 2) {
        perror("Failed to write to I2C bus.");
    }
}

void soft_reset(MechaQMC5883 *sensor) {
    unsigned char config[2];
    config[0] = CTRL_REG2;
    config[1] = 0x80; // Soft reset command
    if (write(sensor->file, config, 2) != 2) {
        perror("Failed to write to I2C bus.");
    }
}

int read_magnetic_data(MechaQMC5883 *sensor, int16_t *x, int16_t *y, int16_t *z) {
    unsigned char buf[7];
    unsigned char reg = OUT_X_L;
    
    // Set register pointer to OUT_X_L
    if (write(sensor->file, &reg, 1) != 1) {
        perror("Failed to set register pointer.");
        return -1;
    }
    
    // Read 7 bytes from the sensor
    if (read(sensor->file, buf, 7) != 7) {
        perror("Failed to read from I2C bus.");
        return -1;
    }

    // Process the data
    *x = (int16_t)(buf[1] << 8 | buf[0]);
    *y = (int16_t)(buf[3] << 8 | buf[2]);
    *z = (int16_t)(buf[5] << 8 | buf[4]);

    // Return status if overflow is detected
    uint8_t overflow = buf[6] & 0x02;
    return overflow << 2;
}

float calculate_azimuth(int _x, int _y, int _z) {
    float x = (float) _x;
    float y = (float) _y;
    float z = (float) _z;
    x = (0.029975 * (x - 1358.944534) + 0.002137 * (y + 366.800852) - 0.005795 * (z - 1175.440578));
    y = (0.002137 * (x - 1358.944534) + 0.036549 * (y + 366.800852) - 0.001469 * (z - 1175.440578));
    z = (-0.005795 * (x - 1358.944534) - 0.001469 * (y + 366.800852) + 0.031086 * (z - 1175.440578));


    x = (1.177870 * (x - 7.020898) - 0.037572 * (y - 2.888513) + 0.195462 * (z - 2.089576));
    y = (-0.037572 * (x - 7.020898) + 0.968025 * (y - 2.888513) - 0.072147 * (z - 2.089576));
    z = (0.195462 * (x - 7.020898) - 0.072147 * (y - 2.888513) + 1.084548 * (z - 2.089576));
    float heading=(atan2(x, y)/0.0174532925);
    //float azimuth = atan2((float)y, (float)x) * 180.0 / M_PI;
    //if (azimuth < 0) {
    //    azimuth += 360;
    //}
    //return azimuth;
     return heading;
}

int main() {
    const char *filename = "/dev/i2c-2"; // I2C device file
    MechaQMC5883 sensor;
    int16_t x, y, z;
    int status;
    float azimuth;

    // Open I2C bus
    if ((sensor.file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    // Specify I2C address
    sensor.address = QMC5883L_ADDR;
    if (ioctl(sensor.file, I2C_SLAVE, sensor.address) < 0) {
        perror("Failed to acquire I2C bus access");
        close(sensor.file);
        return 1;
    }

    // Initialize sensor
    init_qmc5883l(&sensor);

    while (1) {
        // Read magnetic data
        status = read_magnetic_data(&sensor, &x, &y, &z);
        if (status == 0) {
            azimuth = calculate_azimuth(x, y, z);
            printf("X: %d, Y: %d, Z: %d, Azimuth: %.2f\n", x, y, z, azimuth);
        } else {
            printf("Error reading data: %d\n", status);
        }
        sleep(1); // Delay for readability
    }

    close(sensor.file);
    return 0;
}
