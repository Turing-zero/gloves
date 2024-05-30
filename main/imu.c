#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "portmacro.h"
#include "imu.h"

#define TXD_PIN (15) // 如果这些引脚不适合你的硬件，请调整
#define RXD_PIN (16)

static const char *TAG = "UART_RXTX";
struct imu_data imu_status;
float data3[11]; // 用于存储 IMU 数据

#define EX_UART_NUM UART_NUM_2
#define BUF_SIZE (1024)

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // 设置UART参数
    uart_param_config(EX_UART_NUM, &uart_config);
    // 设置UART引脚
    uart_set_pin(EX_UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 安装UART驱动程序，设置RX和TX缓冲区大小
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void update_imu_status(uint8_t* data, int length) {
    for(int index = 0; index < length; index++) {
        if(data[index] == 0x55) { // 找到了值为0x55的字节
            if(data[index+1] == 0x52) {
                imu_status.acc_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 16.0f * 9.8f;
                imu_status.acc_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 16.0f * 9.8f;
                imu_status.acc_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 16.0f * 9.8f;
                imu_status.T_degree = ((short)((data[index + 9] << 8) | data[index + 8])) / 32768.0f * 96.38f + 36.53f;
            } else if(data[index+1] == 0x51) {
                imu_status.omega_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 2000.0f;
                imu_status.omega_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 2000.0f;
                imu_status.omega_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 2000.0f;
                imu_status.voltage = ((short)((data[index + 9] << 8) | data[index + 8])) / 100.0f;
            } else if(data[index+1] == 0x53) {
                imu_status.theta_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 180.0f;
                imu_status.theta_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 180.0f;
                imu_status.theta_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 180.0f;
                imu_status.version = (data[index + 9] << 8) | data[index + 8];
            }

            // 更新 data3 数组
            data3[0] = imu_status.acc_x;
            data3[1] = imu_status.acc_y;
            data3[2] = imu_status.acc_z;
            data3[3] = imu_status.T_degree;
            data3[4] = imu_status.omega_x;
            data3[5] = imu_status.omega_y;
            data3[6] = imu_status.omega_z;
            data3[7] = imu_status.voltage;
            data3[8] = imu_status.theta_x;
            data3[9] = imu_status.theta_y;
            data3[10] = imu_status.theta_z;
        }
    }
}

void uart_task(void *pvParameters) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    while (1) {
        const int rxBytes = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            update_imu_status(data, rxBytes);
        }
    }
    free(data);
}
