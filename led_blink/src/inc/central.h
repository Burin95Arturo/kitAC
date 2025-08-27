// Author: Federico Cañete  
// Date: 08/21/2025
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

void central_task(void *pvParameters);