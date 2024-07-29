#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libserialport.h>

#define BAUD_RATE 115200
#define BUF_SIZE 1024

void error_exit(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

void init_serial_port(struct sp_port **port, const char *port_name) {
    if (sp_get_port_by_name(port_name, port) != SP_OK) {
        error_exit("Cannot find serial port.");
    }

    if (sp_open(*port, SP_MODE_READ) != SP_OK) {
        error_exit("Cannot open serial port.");
    }

    if (sp_set_baudrate(*port, BAUD_RATE) != SP_OK ||
        sp_set_bits(*port, 8) != SP_OK ||
        sp_set_parity(*port, SP_PARITY_NONE) != SP_OK ||
        sp_set_stopbits(*port, 1) != SP_OK ||
        sp_set_flowcontrol(*port, SP_FLOWCONTROL_NONE) != SP_OK) {
        error_exit("Cannot configure serial port.");
    }
}

void read_serial_data(struct sp_port *port) {
    uint8_t data[BUF_SIZE];
    int count = 0;
    time_t start_time = time(NULL);

    while (1) {
        int bytes_read = sp_nonblocking_read(port, data, BUF_SIZE - 1);
        if (bytes_read > 0) {
            data[bytes_read] = '\0';
            count++;

            if (strncmp((char *)data, "IMU", 3) == 0) {
                printf("IMUDATA : %s", data);
            } else if (strncmp((char *)data, "Data1", 5) == 0) {
                printf("data1: %s", data);
            } else if (strncmp((char *)data, "Data2", 5) == 0) {
                printf("data2: %s", data);
            } else if (strncmp((char *)data, "time_diff", 9) == 0) {
                printf("time_diff: %s", data);
            } else {
                printf("type not valid\n");
            }

            time_t current_time = time(NULL);
            if (difftime(current_time, start_time) >= 1.0) {
                double frequency = count / difftime(current_time, start_time);
                printf("Frequency: %.2f messages per second\n", frequency);
                count = 0;
                start_time = current_time;
            }
        }
    }
}

int main() {
    const char *port_name = "COM5";  // 替换为实际的串口名称
    struct sp_port *port;

    init_serial_port(&port, port_name);
    read_serial_data(port);

    sp_close(port);
    sp_free_port(port);

    return 0;
}
