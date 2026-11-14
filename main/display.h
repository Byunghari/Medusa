// display.h - 16x2 character LCD (HD44780) behind a PCF8574 I2C backpack
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// Attach the LCD to an existing I2C master bus and run the HD44780 init sequence.
// addr is the backpack's 7-bit address (usually 0x27 or 0x3F).
esp_err_t lcd_init(i2c_master_bus_handle_t bus, uint8_t addr);

// Clear the screen and move the cursor to the top-left.
void lcd_clear(void);

// Move the cursor: col 0-15, row 0-1.
void lcd_set_cursor(uint8_t col, uint8_t row);

// Write one character at the cursor.
void lcd_putc(char c);

// Write a null-terminated string at the cursor.
void lcd_print(const char *str);
