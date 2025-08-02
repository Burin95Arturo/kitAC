// Author: Burin Arturo
// Date: 20/06/2025

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h" // Para esp_rom_delay_us
#include "inc/pinout.h"

#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo

void lcd_display_task(void *pvParameters);
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_write_string(const char *str);