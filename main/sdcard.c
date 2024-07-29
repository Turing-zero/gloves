#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"


#define EXAMPLE_MAX_CHAR_SIZE    128
#define CONFIG_EXAMPLE_PIN_CMD 35
#define CONFIG_EXAMPLE_PIN_CLK 36
#define CONFIG_EXAMPLE_PIN_D0 37

static const char *TAG1 = "example";

#define MOUNT_POINT "/sdcard"
esp_err_t ret;
sdmmc_card_t *card;
int start_flag=1;

static esp_err_t s_example_write_file(const char *path, char *data)
{
    // ESP_LOGI(TAG1, "Opening file %s", path);
    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG1, "Failed to open file for writing");
        
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    // ESP_LOGI(TAG1, "File written");

    return ESP_OK;
}
static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG1, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG1, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG1, "Read from file: '%s'", line);

    return ESP_OK;
}

void sdmmc_init()
{

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG1, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    // ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG1, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG1, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG1, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);


}
void generate_file_name(char *file_name, size_t max_len) {
    time_t now;
    struct tm timeinfo;
    time(&now); // 获取当前时间
    localtime_r(&now, &timeinfo); // 将时间转换为本地时间
    strftime(file_name, max_len, MOUNT_POINT"/log_%Y%m%d_%H%M%S.txt", &timeinfo); // 格式化时间并生成文件名
}



void sd_write(double data1[5][5], double data2[5], float data3[11],const char *filename)
{
    if (filename == NULL || strlen(filename) == 0) {
        ESP_LOGE(TAG1, "Invalid filename");
        return;
    }
    // 动态使用传入的文件名
    char file_hello[30]; // 假设路径不会超过256字符
    snprintf(file_hello, sizeof(file_hello), "%s/%s", MOUNT_POINT, filename);

    char data[EXAMPLE_MAX_CHAR_SIZE];  // 增大缓冲区以适应写入整个数组的数据
    memset(data, 0, sizeof(data));
    
    // 获取当前时间并格式化
    time_t now;
    struct tm timeinfo;
    char time_str[64];
    time(&now); // 获取当前时间
    localtime_r(&now, &timeinfo); // 将时间转换为本地时间
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo); // 格式化时间字符串

    // 写入日期时间
    snprintf(data, sizeof(data), "%s\n", time_str);
    int ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return; // 如果写入失败，返回
    }

    // 写入 data1 数组
    for (int row = 0; row < 5; row++) {
        memset(data, 0, sizeof(data)); // 清空缓冲区
        snprintf(data, sizeof(data), "Data1[%d]: %.3f, %.3f, %.3f, %.3f, %.3f\n",
                 row, data1[row][0], data1[row][1], data1[row][2], data1[row][3], data1[row][4]);
        ret = s_example_write_file(file_hello, data);
        if (ret != ESP_OK) {
            return; // 如果写入失败，返回
        }
    }

    // 写入 data2 数组
    memset(data, 0, sizeof(data)); // 清空缓冲区
    snprintf(data, sizeof(data), "Data2: %.3f, %.3f, %.3f, %.3f, %.3f\n",
             data2[0], data2[1], data2[2], data2[3], data2[4]);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return; // 如果写入失败，返回
    }

    // 写入 data3 数组
    memset(data, 0, sizeof(data)); // 清空缓冲区
    snprintf(data, sizeof(data), "Data3: %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
             data3[0], data3[1], data3[2], data3[3], data3[4], data3[5], data3[6], data3[7], data3[8], data3[9], data3[10]);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return; // 如果写入失败，返回
    }
}