#include "inc/hall.h"

static bool getHallSensorState(void){
    
    return gpio_get_level(HALL_PIN);
}

void hall_sensor_task(void *pvParameters) {

    data_t hall_data;
    
    while (1) {

        if (xSemaphoreTake(hall_semaphore, portMAX_DELAY) == pdTRUE) {
        
//            gpio_set_level(LED_PIN, getHallSensorState());
            hall_data.origen = SENSOR_HALL;
            hall_data.hall_on_off = getHallSensorState();
        
            if (xQueueSend(central_queue, &hall_data, (TickType_t)0) != pdPASS) {
                printf("No se pudo enviar el HALL a la cola.");
            }
            
            printf("Estado del sensor de efecto Hall: %d", hall_data.hall_on_off);  

        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Leer cada 500 ms
    }
        
}