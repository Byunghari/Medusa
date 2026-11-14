idf_component_register(SRCS "main.c"
                            "display.c"
                            "fdc.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES freertos         # freertos/FreeRTOS.h, freertos/task.h
                                     esp_rom          # esp_rom_sys.h (esp_rom_delay_us)
                                     esp_driver_uart  # driver/uart.h
                                     esp_driver_i2c)  # driver/i2c_master.h
# <stdio.h>, <string.h> and <math.h> come from newlib/libm, which ESP-IDF links automatically.
