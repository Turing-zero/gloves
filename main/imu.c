#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "portmacro.h"

#define TXD_PIN (17) // 如果这些引脚不适合你的硬件，请调整
#define RXD_PIN (16)

static const char *TAG = "UART_RXTX";
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
    } imu_status;


#define EX_UART_NUM UART_NUM_2
#define BUF_SIZE (1024)

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
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

void print_imu_data() {
    // ESP_LOGI("imu", "imu acc: %f, %f, %f, %f", imu_status.acc_x, imu_status.acc_y, imu_status.acc_z, imu_status.T_degree);
    // ESP_LOGI("imu", "imu omega: %f, %f, %f, %f", imu_status.omega_x, imu_status.omega_y, imu_status.omega_z, imu_status.voltage);
    // ESP_LOGI("imu", "imu theta: %f, %f, %f, %f", imu_status.theta_x, imu_status.theta_y, imu_status.theta_z, imu_status.version);
}
void uart_task(void *pvParameters) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    // 假设每个字节转换成16进制需要最多3个字符（包括空格），+1 为字符串结束符
    char *hex_str = (char*)malloc(BUF_SIZE * 3 + 1); 
    while (1) {
        // 读取数据
        const int rxBytes = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            int idx = 0; 
           for(int index = 0; index < rxBytes; index++) {
                if(data[index] == 0x55) {// 找到了值为0x55的字节
                    if(data[index+1]==0x52)
                    {
                        imu_status.acc_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 16.0f * 9.8f;
                        imu_status.acc_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 16.0f * 9.8f;
                        imu_status.acc_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 16.0f * 9.8f;
                        imu_status.T_degree = ((short)((data[index + 9] << 8) | data[index + 8])) / 32768.0f * 96.38f + 36.53f;
                    }
                    else if(data[index+1]==0x51)
                    {
                        imu_status.omega_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 2000.0f;
                        imu_status.omega_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 2000.0f;
                        imu_status.omega_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 2000.0f;
                        imu_status.voltage = ((short)((data[index + 9] << 8) | data[index + 8])) / 100.0f;
                    }
                    else if(data[index+1]==0x53)
                    {
                        imu_status.theta_x = ((short)((data[index + 3] << 8) | data[index + 2])) / 32768.0f * 180.0f;
                        imu_status.theta_y = ((short)((data[index + 5] << 8) | data[index + 4])) / 32768.0f * 180.0f;
                        imu_status.theta_z = ((short)((data[index + 7] << 8) | data[index + 6])) / 32768.0f * 180.0f;
                        imu_status.version = (data[index + 9] << 8) | data[index + 8];
                    }

                
            }  
              
        }
    }
    print_imu_data();
}
}