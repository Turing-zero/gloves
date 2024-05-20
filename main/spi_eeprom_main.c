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
#include "imu.c"
#include "esp_timer.h"


static int64_t last_time = 0;

#if CONFIG_IDF_TARGET_ESP32S3
#   define AD7689_HOST      SPI2_HOST
#   define PIN_NUM_MISO     13  // Adjust based on your board configuration
#   define PIN_NUM_MOSI     11  // Adjust based on your board configuration
#   define PIN_NUM_CLK      12  // Adjust based on your board configuration
#   define PIN_NUM_CS       10  // Adjust based on your board configuration
#   define PIN_NUM_CS2      9  // 为第二个从机定义CS引脚 

#else
#error "Target not supported"
#endif
 

// 根据ESP32S3的引脚分配选择RX和TX引脚
#define TXD1_PIN (41)  
#define RXD1_PIN (42)
#define PIN_4 GPIO_NUM_4
#define PIN_5 GPIO_NUM_5
#define PIN_6 GPIO_NUM_6

int64_t time=0;
int64_t time2=0;
int row,col=0;
double data1[5][5]={0};
double data2[5]={0};
double voltage2 =0;
double voltage1 =0;
double count=0;
void gpio_init() {
    esp_rom_gpio_pad_select_gpio(PIN_4);
    esp_rom_gpio_pad_select_gpio(PIN_5);
    esp_rom_gpio_pad_select_gpio(PIN_6);
    gpio_set_direction(PIN_4, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_6, GPIO_MODE_OUTPUT);
}
void uart1_init() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
}
int64_t get_time()
{
    int64_t time_diff=0;
    int64_t start_time = esp_timer_get_time(); // 获取当前时间戳
    if (last_time != 0) {
        time_diff = start_time - last_time; // 计算两次采样之间的时间差
        // ESP_LOGI(TAG, "Time difference between samples: %lld microseconds", time_diff);

    }
    last_time=start_time;
    return time_diff;
}

void send_data_over_uart() {
    char buffer[256];
    time2=get_time();
    // 发送IMU数据
    int len = snprintf(buffer, sizeof(buffer), "IMU:%f, %f, %f；%f, %f, %f；%f, %f, %f\n",
                       imu_status.acc_x, imu_status.acc_y, imu_status.acc_z,
                       imu_status.omega_x, imu_status.omega_y, imu_status.omega_z,
                       imu_status.theta_x, imu_status.theta_y, imu_status.theta_z);
    uart_write_bytes(UART_NUM_1, buffer, len);

    // 发送data1数组
    for (int i = 0; i < 5; i++) {
        len = snprintf(buffer, sizeof(buffer), "Data1:%d: %.3f, %.3f, %.3f, %.3f, %.3f\n",
                       i, data1[i][0], data1[i][1], data1[i][2], data1[i][3], data1[i][4]);
        uart_write_bytes(UART_NUM_1, buffer, len);
    }

    // 发送data2数组
    len = snprintf(buffer, sizeof(buffer), "Data2: %.3f, %.3f, %.3f, %.3f, %.3f,%.3f\n",
                   data2[0], data2[1], data2[2], data2[3], data2[4],count);
    uart_write_bytes(UART_NUM_1, buffer, len);

    //send time
    len = snprintf(buffer, sizeof(buffer), "time_diff: %lld\n",time2);
    uart_write_bytes(UART_NUM_1, buffer, len);
}


void decoder(int i) {
    if (i < 0 || i > 5) {
        ESP_LOGE("DECODER", "Input must be between 0 and 5.");
        return;
    }

    gpio_set_level(PIN_4, (i & 0b001) ? 1 : 0);
    gpio_set_level(PIN_5, (i & 0b010) ? 1 : 0);
    gpio_set_level(PIN_6, (i & 0b100) ? 1 : 0);
    // ESP_LOGI(TAG, "decoder :%d",i);
}

int x=0;
int flagisp=0;
uint8_t tx_data[2] = {0xFF, 0xFF};
uint8_t tx_data2[2] = {0xFF, 0xFF}; // 假设发送给第二个从机的数据
uint8_t rx_data2[2]; // 接收来自第二个从机的数据的缓冲区
uint8_t rx_data[9] = {0}; // 用于接收数据的缓冲区，大小与发送数据一致
int num=0;

uint16_t combineTo16Bit(uint8_t MSB, uint8_t LSB) {
    // 直接进行位移和位或操作，无需显式类型转换
    return (MSB << 8) | LSB;
}
double calculateVoltage(uint16_t binaryValue) {
    const double maxVoltage = 4.096;
    const uint16_t maxValue = 0xFFFF;

    return (binaryValue * maxVoltage) / maxValue;
}

int spi_read(int i)
{
    

    // 现有的采样代码
    tx_data[0] = 0xF1; // 发送的数据
    tx_data[1] = 0x20; // 发送的数据
    tx_data2[0] = 0xF1;
    tx_data2[1] = 0x20;
    tx_data[0] += i * 2;
    tx_data2[0] += i * 2;

    if (i == 5) {
        row++;
        decoder(row);
        if (row > 4) {
            row = 0;
        }
    }

    
    return 0;
}
void debug_msg()
{
     time=get_time();
    for(int i=0;i<5;i++)
        {
            for(int j=0;j<5;j++)
            {
                printf("%.3f ", data1[i][j]);
            }
            printf("\n ");
        }
        
        for(int i=0;i<5;i++)
        {
            printf("%.3f ", data2[i]);
        }
        printf("\n ");
        printf("%.3f ", count);
        printf("\n");
        printf("%lld ", time);
        printf("\n");
}

void init()
{
    uart1_init();
    gpio_init();
    tx_data[0] = 0xF1; // 发送的数据
    tx_data[1] =0x20; // 发送的数据
    tx_data2[0]=0xF1;
    tx_data2[1]=0x20;
}

void app_main(void)
{
    init();

    esp_err_t ret;
    ESP_LOGI(TAG, "Initializing SPI bus...");
    
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    // Initialize the SPI bus
    ret = spi_bus_initialize(AD7689_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_handle_t spi;
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1*1000*1000,  // Clock out at 1 MHz
        .mode = 3,                      // SPI mode 1 for AD7689
        .spics_io_num = PIN_NUM_CS,     // CS pin
        .queue_size = 7,                // We want to be able to queue 7 transactions at a time
    };
    spi_device_handle_t spi2; // 定义第二个从机的设备句柄
    spi_device_interface_config_t devcfg2 = {
    .clock_speed_hz = 1*1000*1000,  // 第二个从机的时钟速度，例如1 MHz
    .mode = 3,                       // 第二个从机的SPI模式，根据需要进行调整
    .spics_io_num = PIN_NUM_CS2,     // 第二个从机的CS引脚
    .queue_size = 7,                 // 能够同时排队的事务数
    };

    // Attach the AD7689 to the SPI bus 
    ret = spi_bus_add_device(AD7689_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(AD7689_HOST, &devcfg2, &spi2);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "AD7689 device initialized.");
    uart_init();
    xTaskCreate(uart_task, " ", 4096, NULL, 5, NULL);



    while (1) {
        count=count+0.1;
        spi_read(num);
        if (num<5)
         {
            
            data1[num][row]=voltage1;
            data2[num]=voltage2;
            num++;
         }
        else
        num=0;
         

        spi_transaction_t t2;
        memset(&t2, 0, sizeof(t2));  // 清零结构体
        t2.length = 8 *4; // 事务长度，单位为位
        t2.tx_buffer = tx_data2;      // 发送缓冲区
        t2.rx_buffer = rx_data2;      // 接收缓冲区

        // 发送数据并接收来自第二个SPI从机的数据

        
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 31; // 24 bits
        t.tx_buffer = tx_data; // 发送缓冲区
        t.rx_buffer = rx_data; // 接收缓冲区

        // 发送数据并接收数据
        esp_err_t ret = spi_device_transmit(spi, &t); // SPI 设备句柄 `spi` 必须已经通过 `spi_bus_add_device` 初始化
        ESP_ERROR_CHECK(ret);
        ret = spi_device_transmit(spi2, &t2);
        ESP_ERROR_CHECK(ret);
        // 打印发送的数据
        uint16_t combinedValue1 = combineTo16Bit(rx_data[0], rx_data[1]);  // 假设 MSB 在第二字节，LSB 在第一字节
        voltage1 = calculateVoltage(combinedValue1);
        uint16_t combinedValue2 = combineTo16Bit(rx_data2[0], rx_data2[1]);  // 假设 MSB 在第二字节，LSB 在第一字节
        voltage2 = calculateVoltage(combinedValue2);



        // ESP_LOGI(TAG, "Sent data: %02X %02X %02X", tx_data[0], tx_data[1], tx_data[2]);
        // ESP_LOGI(TAG, "Sent data: %02X %02X %02X", tx_data2[0], tx_data2[1], tx_data2[2]);

        // // 打印接收到的数据
        // ESP_LOGI(TAG, "Received data %d : %02X %02X %02X %02X %.3f", num,rx_data[0], rx_data[1], rx_data[2], rx_data[3], voltage1);
        // ESP_LOGI(TAG, "Received data %d : %02X %02X %02X %02X %.3f ",num, rx_data2[0], rx_data2[1], rx_data2[2], rx_data2[3], voltage2);
        // ESP_LOGI(TAG, "num:%d",num);
        if(num==0&&row==0)
        {
            debug_msg(); 
            send_data_over_uart();          
        } 

    //vTaskDelay(pdMS_TO_TICKS(10)); // 延时等待下一次传输    
        }
   
}


