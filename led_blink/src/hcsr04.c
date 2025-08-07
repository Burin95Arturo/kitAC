// Author: Burin Arturo
// Date: 11/04/2025

#include "inc/hcsr04.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "esp_log.h"
#include "inc/program.h"


// --- Tag para el logging del ESP-IDF ---
static const char *TAG = "HCSR04";

long getDistance() {
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    long startTime = esp_timer_get_time();
    long endTime = startTime;

    while (gpio_get_level(ECHO_PIN) == 0) {
        startTime = esp_timer_get_time();
    }

    while (gpio_get_level(ECHO_PIN) == 1) {
        endTime = esp_timer_get_time();
    }

    long duration = endTime - startTime;
    long distance = duration / 58;

    return distance;
}

void hc_sr04_task(void *pvParameters) {
    long distance;
    float current_height_m;
    while (1) {
        distance = getDistance();
        printf("Distancia: %ld cm\n", distance);

        if (distance < 20) {
            gpio_set_level(LED_PIN, 1);
        } else {
            gpio_set_level(LED_PIN, 0);
        }

        current_height_m = (long) distance;
        // Enviar el peso calculado a la cola
        if (xQueueSend(height_queue, &current_height_m, (TickType_t)0) != pdPASS) {
            ESP_LOGE(TAG, "No se pudo enviar el peso a la cola.");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
