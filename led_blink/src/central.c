// Author: Federico Cañete  
// Date: 08/21/2025

#include "inc/central.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"


void central_task(void *pvParameters) {

    while (1) {
        // Da el semáforo binario para que tasktest pueda funcionar
        xSemaphoreGive(acelerometro_semaphore);

        // Leer de la cola central_queue
        data_t received_data;
        if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[Central] Origen: %d, Altura: %ld, Peso: %.2f, OnOff: %d\n",
                received_data.origen,
                received_data.altura,
                received_data.peso,
                received_data.on_off);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}