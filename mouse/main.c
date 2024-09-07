#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "transmitter.h"
#include "receiver.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
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

float pitch_gyro = 0.0;
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
    float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az)) * -180.0 / M_PI;

    // Simple integration for yaw (this is a rough estimate)
    static float prev_gz = 0;
    static float delta_t = 0.01; // Time interval in seconds (update this based on your setup)
    pitch_gyro += gy * delta_t;
    *pitch = 0.0 * pitch_acc + 1 * pitch_gyro;
    *yaw += gz * delta_t; // Update yaw using gyro data
    if (*yaw > 360.0) *yaw -= 360.0;
    if (*pitch > 360.0) *pitch -= 360.0;
    //if (*yaw < 0.0) *yaw += 360.0;

    prev_gz = gz;
}

void* handshake (void* data)
{
    transmitter_t* initial_transmitter = (transmitter_t*)malloc(sizeof(transmitter_t));
    strcpy(initial_transmitter->ip, "192.168.1.255");
    initial_transmitter->port = 12345;
    init_broadcast_sender(initial_transmitter);

    while (1)
    {
        send_broadcast_message(initial_transmitter, "CONNRQ");
        sleep(1);
    }
}

int main()
{
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

    pthread_t handshaker;
    receiver_t* response_receiver = (receiver_t*)malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));

    while (1)
    {
        int failedToCreateThread = pthread_create(&handshaker, NULL, handshake, (void*) NULL);
        if (failedToCreateThread)
        {
            printf("Failed to create a thread... \n");
            return false;
        }

        init_unicast_listener(response_receiver);
        poller->client.msg = "ACCRQ";
//        poller->client = client;
        wait_for_message(response_receiver, poller);

        if (poller->flag)
        {
            printf("Received ACCRQ\n");
            pthread_cancel(handshaker);
            transmitter_t* acceptor = (transmitter_t*)malloc(sizeof(transmitter_t));
            strcpy(acceptor->ip, inet_ntoa(response_receiver->cliaddr.sin_addr));
            acceptor->port = ntohs(response_receiver->cliaddr.sin_port);
            acceptor->port = 12345;
            init_unicast_sender(acceptor);

            printf("Sending ACCRQ to %s:%d\n", acceptor->ip, acceptor->port);
            send_message(acceptor, "ACK");
            while (1) //infinite loop would later be replaced by subscription to stop message.
            {
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
                atomic_store(&acceptor->is_initialized, false);
                close(acceptor->sockfd);
                init_unicast_sender(acceptor);
                printf("Enter new mouse position\n");
                char cmd[20] = "MVMSE ";
                char pos[20];
                char x_str[10];
                char y_str[10];
                //scanf("%s", pos);
                int x = (yaw * 70.0) + 750;
                int y = (pitch * 70.0) + 750;
                sprintf(x_str, "%d", x);
                sprintf(y_str, "%d", y);
                strcpy(pos, x_str);
                strcat(pos, ",");
                strcat(pos, y_str);
                strcat(cmd, pos);
                send_message(acceptor, cmd);
            }
        }
    }
}
