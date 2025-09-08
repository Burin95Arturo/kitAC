#include "inc/program.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "esp_log.h"

// static const char *TAG_BUTTON = "BUTTONS";

    
// Tags para logs
// static const char *TAG_BTN = "BUTTON_TASK";

void button_task(void *pvParameters);