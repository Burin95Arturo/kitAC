#include "inc/tasktest.h"


void tasktest(void *pvParameters) {

    // static data_t test;
    // test.origen = TEST_TASK;
    // test.altura = 123;
    // // test.peso = 45.67f;
    // test.hall_on_off = true;
    // test.ir_on_off = true;

    // while (1) {
        
    //     // Tomar el semáforo binario acelerometro_semaphore. Si no puede, se bloquea.
    //     if (xSemaphoreTake(task_test_semaphore, portMAX_DELAY) == pdTRUE) {
            
    //         test.altura += 1;
    //         // test.peso += 0.1f;
    //         test.hall_on_off = !test.hall_on_off;
    //         test.ir_on_off = !test.ir_on_off;
            
    //         if (xQueueSend(central_queue, &test, (TickType_t)0) != pdPASS) {
    //         ESP_LOGI("TaskTest", "No se pudo enviar el peso a la cola.");
    //         test.hall_on_off = !test.hall_on_off;

    //         }

    //     }

        

    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}