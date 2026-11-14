#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/i2c_master.h"
#include "display.h"

#define UART_PORT     UART_NUM_2
#define UART_TX_PIN   17
#define UART_RX_PIN   16
#define UART_BAUD     115200
#define UART_RX_BUF   1024        // must be bigger than the 128-byte hardware FIFO

#define I2C_SDA_PIN   21
#define I2C_SCL_PIN   22
#define LCD_ADDR      0x27        // try 0x3F if lcd_init fails

#define LCD_COLS      16
#define LINE_MAX      (LCD_COLS * 2)   // 32 characters fit on a 16x2

// Show up to 32 characters: first 16 on the top row, the rest on the bottom.
static void show_line(const char *line)
{
    lcd_clear();                                   // also moves cursor to top-left
    for (int i = 0; i < LCD_COLS && line[i] != '\0'; i++) {
        lcd_putc(line[i]);
    }
    if (strlen(line) > LCD_COLS) {
        lcd_set_cursor(0, 1);
        lcd_print(line + LCD_COLS);                // pointer to character 17 onward
    }
}

void app_main(void)
{
    // ---- UART2: install, configure, pins ----
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUF, 0, 0, NULL, 0));
    uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,    // only TX, RX, GND wired
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // ---- I2C bus + LCD ----
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = I2C_SDA_PIN,
        .scl_io_num        = I2C_SCL_PIN,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
    ESP_ERROR_CHECK(lcd_init(bus, LCD_ADDR));
    show_line("Ready");

    // ---- Main loop: collect bytes into a line, show it on Enter ----
    uint8_t rx[64];                  // raw bytes from one read
    char line[LINE_MAX + 1];         // +1 for the '\0'
    size_t len = 0;                  // characters in 'line' so far

    while (1) {
        int n = uart_read_bytes(UART_PORT, rx, sizeof(rx), pdMS_TO_TICKS(100));

        for (int i = 0; i < n; i++) {
            char c = (char)rx[i];

            if (c == '\r' || c == '\n') {            // Enter: line finished
                if (len > 0) {
                    line[len] = '\0';
                    show_line(line);
                    uart_write_bytes(UART_PORT, "OK\n", 3);   // tell the Pi it worked
                    len = 0;
                }
            } else if (c == 0x08 || c == 0x7F) {     // Backspace
                if (len > 0) {
                    len--;
                }
            } else if (c >= 0x20 && c <= 0x7E) {     // printable characters only
                if (len < LINE_MAX) {                // never write past the buffer
                    line[len++] = c;
                }
            }
        }
    }
}