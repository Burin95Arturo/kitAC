#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "inc/program.h"
#include <freertos/queue.h>

static bool getIrSensorState(void);

void ir_sensor_task(void *pvParameters);