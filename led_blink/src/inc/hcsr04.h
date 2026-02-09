#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "esp_log.h"
#include "inc/program.h"
#include <freertos/queue.h>

// --- Tag para el logging del ESP-IDF ---
// static const char *TAG_3 = "HCSR04";

#define TIME_MAX_DISTANCE 30000000 // Tiempo máximo para medir distancia (en microsegundos) -> 30 segundos
#define MIN_DISTANCE 0
#define MAX_DISTANCE 300

void hc_sr04_task(void *pvParameters);