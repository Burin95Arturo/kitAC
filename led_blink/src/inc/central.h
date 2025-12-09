#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "inc/program.h"
#include "esp_log.h"
#include "freertos/queue.h"

#define MUESTRAS_PROMEDIO 100

void central_task(void *pvParameters);