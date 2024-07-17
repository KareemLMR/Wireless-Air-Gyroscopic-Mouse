# Overview
This project aims to make a wireless air gyroscopic mouse for PCs. With this mouse you can hover in the air with the mouse and you will see the cursor moving in the same direction you move the mouse in.
And here are all the diagrams describing my project:.

![Alt text](General-Block-Diagram.jpg?raw=true "General System Block Diagram")
![Alt text](Sequence-diagram.png?raw=true "Communication Protocol Sequence Diagram")
![Alt text](Security.jpg?raw=true "Driver Configuring and Exchanging Certificates Mode Diagram")

# Target Build System
In this project, I will use Yocto as my build system.

# Hardware Platform
Beaglebone Black
IMU MPU6050

# Open Source Projects Used
OpenSSL
MPU6050 Driver

# Previously Discussed Content
In this project I will use aesdsocket partially.

# New Content
I will add a security part encrypting the socket communication by OpenSSL based on a certificate to be generated and exchanged over an ethernet or USB cable during configuring the PC to work with the mouse driver just like PS Wireless Joystick.

I will also add a protocol implementation where the mouse first will broadcast to all PCs on the network offering service to connect and the first one to reply with the IP, Port and parameters will be accepted.

I will also use X11 for controlling mouse cursor position on the GUI

# Shared Material
N/A.

# Source Code Organization
Buildroot or Yocto Repository will be hosted at [Yocto Meta Layer](https://github.com/cu-ecen-aeld/final-project-KareemLMR)

Wireless Air Gyroscopic Mouse source code will be hosted in in a repository at [Wireless Air Gyroscopic Mouse](https://github.com/KareemLMR/Wireless-Air-Gyroscopic-Mouse)

## Team project members:
Kareem Ibrahim: All of the Project

# Schedule Page
[Project Schedule](https://github.com/users/KareemLMR/projects/1/views/1)