#include "inc/central.h"


void central_task(void *pvParameters) {

    while (1) {
        // Da el semáforo binario para que tasktest pueda funcionar
        xSemaphoreGive(task_test_semaphore);
        xSemaphoreGive(peso_semaphore);
        xSemaphoreGive(hall_semaphore);
        xSemaphoreGive(ir_semaphore);  
        xSemaphoreGive(altura_semaphore);
        xSemaphoreGive(button_semaphore);
        
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