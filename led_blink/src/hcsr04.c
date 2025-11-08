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

static long getDistance(void);

static long getDistance(void) {
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
    //float current_height_m;
    
    while (1) {
        distance = getDistance();
        printf("Distancia: %ld cm\n", distance);

        //Poner indicacion de leds

        //Envio altura a queue
        if (xQueueSend(height_queue, &distance, 0) != pdPASS) {
            ESP_LOGI(TAG, "Fallo al enviar la altura (%.2ld cm) a la cola. Cola llena.", distance);
        } else {
             // Opcional: Confirmar el envío
             //ESP_LOGI("HCSR04", "Altura enviada: %.2f m", current_height_m);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
