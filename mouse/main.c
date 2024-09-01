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


//#define PORT 12345
//#define BROADCAST_ADDR "192.168.7.3"  // Adjust this to your network's broadcast address
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

    pthread_t handshaker;
    receiver_t* response_receiver = (receiver_t*)malloc(sizeof(receiver_t));
    poller_t* poller = (poller_t*)malloc(sizeof(poller_t));
    //client_t* client = (client_t*)malloc(sizeof(client_t));

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
                atomic_store(&acceptor->is_initialized, false);
                close(acceptor->sockfd);
                init_unicast_sender(acceptor);
                printf("Enter new mouse position\n");
                char cmd[20] = "MVMSE ";
                char pos[20];
                char x_str[10];
                char y_str[10];
                //scanf("%s", pos);
                sprintf(x_str, "%d", x);
                sprintf(y_str, "%d", y);
                strcpy(pos, y_str);
                strcat(pos, ",");
                strcat(pos, x_str);
                strcat(cmd, pos);
                send_message(acceptor, cmd);
            }
        }
    }
    return 0;
}
