#include "inc/tasktest.h"


void tasktest(void *pvParameters) {

    static data_t test;
    test.origen = TEST_TASK;
    test.altura = 123;
    test.peso = 45.67f;
    test.on_off = true;

    while (1) {
        
        if (xQueueSend(central_queue, &test, (TickType_t)0) != pdPASS) {
            ESP_LOGE(TAG, "No se pudo enviar el peso a la cola.");
        }
    
        test.altura += 1;
        test.peso += 0.1f;
        test.on_off = !test.on_off;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}