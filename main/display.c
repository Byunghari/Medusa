// lcd.c - 16x2 character LCD (HD44780) behind a PCF8574 I2C backpack
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"   // esp_rom_delay_us(): exact short waits

// ---- PCF8574 pin map: what each bit of the byte we send controls ----
// bit:  7   6   5   4   3          2    1    0
// pin:  D7  D6  D5  D4  backlight  EN   RW   RS
#define LCD_RS         0x01   // 0 = command, 1 = character
#define LCD_RW         0x02   // always 0 (we only write)
#define LCD_EN         0x04   // pulse 1 -> 0 to make the LCD take the data
#define LCD_BACKLIGHT  0x08   // 1 = backlight on

#define LCD_I2C_SPEED_HZ  100000   // 100 kHz, safe for every backpack
#define LCD_TIMEOUT_MS    100

// The device handle we get back from the I2C driver. "static" keeps it
// private to this file - main.c never touches it directly.
static i2c_master_dev_handle_t lcd_dev;

// ---- Layer 2: set all 8 backpack pins at once ----
static esp_err_t lcd_write_pins(uint8_t pins)
{
    return i2c_master_transmit(lcd_dev, &pins, 1, LCD_TIMEOUT_MS);
}

// ---- Layer 3: send 4 bits (nibble = the low 4 bits of 'nibble') ----
static esp_err_t lcd_send_nibble(uint8_t nibble, uint8_t rs)
{
    // Move the 4 data bits up into D4-D7, add backlight and RS.
    uint8_t pins = (uint8_t)((nibble << 4) | LCD_BACKLIGHT | rs);

    // EN high...
    esp_err_t err = lcd_write_pins(pins | LCD_EN);
    if (err != ESP_OK) {
        return err;
    }
    // ...then EN low: the LCD grabs D4-D7 on this falling edge.
    // No extra delay needed: one I2C byte at 100 kHz takes ~180 us,
    // which is already longer than the LCD needs per step (~40 us).
    return lcd_write_pins(pins);
}

// ---- Layer 4: send a full byte as two nibbles, high half first ----
static esp_err_t lcd_send_byte(uint8_t value, uint8_t rs)
{
    esp_err_t err = lcd_send_nibble(value >> 4, rs);   // high half
    if (err != ESP_OK) {
        return err;
    }
    return lcd_send_nibble(value & 0x0F, rs);          // low half
}

static void lcd_command(uint8_t cmd)
{
    lcd_send_byte(cmd, 0);        // RS = 0 -> command
}

// ---- Public functions ----
void lcd_putc(char c)
{
    lcd_send_byte((uint8_t)c, LCD_RS);   // RS = 1 -> character
}

void lcd_print(const char *str)
{
    while (*str != '\0') {
        lcd_putc(*str);
        str++;
    }
}

void lcd_clear(void)
{
    lcd_command(0x01);
    esp_rom_delay_us(2000);       // clear is slow: needs ~1.5 ms
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    // Row 0 starts at LCD address 0x00, row 1 at 0x40.
    // Command 0x80 means "set cursor address", so OR the address in.
    uint8_t row_start = (row == 0) ? 0x00 : 0x40;
    lcd_command((uint8_t)(0x80 | (row_start + col)));
}

// ---- Layer 1 + Layer 5: attach to the bus and wake the LCD up ----
esp_err_t lcd_init(i2c_master_bus_handle_t bus, uint8_t addr)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = LCD_I2C_SPEED_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &lcd_dev);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));   // LCD needs >40 ms after power-up

    // Wake-up sequence: the LCD might be in any state at boot, so force
    // 8-bit mode three times, then switch to 4-bit mode.
    // The first send also tells us if the LCD is actually at this address.
    err = lcd_send_nibble(0x03, 0);
    if (err != ESP_OK) {
        return err;               // nobody answered: wrong address or wiring
    }
    esp_rom_delay_us(5000);
    lcd_send_nibble(0x03, 0);
    esp_rom_delay_us(200);
    lcd_send_nibble(0x03, 0);
    esp_rom_delay_us(200);
    lcd_send_nibble(0x02, 0);     // now in 4-bit mode
    esp_rom_delay_us(200);

    lcd_command(0x28);            // 4-bit, 2 lines, 5x8 font
    lcd_command(0x0C);            // display on, cursor off, blink off
    lcd_clear();
    lcd_command(0x06);            // cursor moves right after each character

    return ESP_OK;
}
