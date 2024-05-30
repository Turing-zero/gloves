

#ifndef IMU_H
#define IMU_H

#include <stdint.h>

struct imu_data {
    float acc_x;
    float acc_y;
    float acc_z;
    float T_degree;
    float omega_x;  // degree/s
    float omega_y;
    float omega_z;
    float voltage;
    float theta_x;
    float theta_y;
    float theta_z;
    float version;
};

extern struct imu_data imu_status; // 声明全局变量 imu_status
extern float data3[11]; // 声明全局变量 data3

void uart_init(void); // 初始化UART
void uart_task(void *pvParameters); // UART任务

#endif // IMU_H
