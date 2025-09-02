#include "inc/ir.h"

static bool getIrSensorState(void){
    
    return gpio_get_level(IR_PIN);
}

void ir_sensor_task(void *pvParameters) {

    data_t ir_data;
    
    while (1) {

        if (xSemaphoreTake(hall_semaphore, portMAX_DELAY) == pdTRUE) {
        
            ir_data.origen = SENSOR_IR;
            ir_data.on_off = getIrSensorState();
        
            if (xQueueSend(central_queue, &ir_data, (TickType_t)0) != pdPASS) {
                printf( "No se pudo enviar IR a la cola.");
            }
            
            printf("Estado del sensor IR: %d", ir_data.on_off);  

        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Leer cada 500 ms
    }

}