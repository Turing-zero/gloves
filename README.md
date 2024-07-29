| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

## SPI master half duplex EEPROM example

 This code demonstrates how to use the SPI master half duplex mode to read/write a AT93C46D
 EEPROM (8-bit mode). There is also an Kconfig option `EXAMPLE_USE_SPI1_PINS` allowing use the
 SPI1 (bus with code Flash connected on official modules).

### Connections

For different chip and host used, the connections may be different. Here show a example diagram of hardware connection, you can freely change the GPIO defined in start of `main/spi_eeprom_main.c` and change the hardware relatively.

|      | ESP32 |
| ---- | ----- |
| Host | SPI1  |
| VCC  | 3.3V  |
| GND  | GND   |
| DO   | 7     |
| DI   | 8     |
| SK   | 6     |
| CS   | 13    |
| ORG  | GND   |

### Notes
目前各个采样独立运作正常，但但手套弯曲时各个压力采集点会出现侧向的应力照成无法识别正确压力值。
嵌入式已有功能：
    手指弯曲度采集
    手指个点压力采集
    imu数据采集
    sd卡数据写入
    uart数据传输（波特率：115200）
上位机功能（test.py）：
    弯曲度识别
    各点压力识别
    imu数据显示
    采集数据可视化
    各点数据分档
使用说明：test.py有个输入框，在输入框输入：“init kg[i][j] value"可修改[i][j]的压力点分档数据value是只当前受到的重量大小；当输入”init i radius“是修改第i个弯曲度传感器当前收到的数据定为半径为radius。
    