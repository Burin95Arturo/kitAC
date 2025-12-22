#include "inc/hall.h"

static bool getHallSensorState(void){
    
    return gpio_get_level(HALL_PIN);
}

void hall_sensor_task(void *pvParameters) {

    central_data_t hall_data;
    hall_data.origen = SENSOR_HALL;
    hall_data.hall_on_off = false;
    static uint32_t received_request_id; 


    while (1) {

        /* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación

		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);
        
//            gpio_set_level(INTERNAL_LED_PIN, getHallSensorState());
            
        //hall_data.hall_on_off = getHallSensorState();
        hall_data.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido
        
                //Sólo para simulación:
                if (received_request_id % 12 == 0){
                    hall_data.hall_on_off = !hall_data.hall_on_off;
                }
        
        if (xQueueSend(central_queue, &hall_data, (TickType_t)0) != pdPASS) {
            printf("No se pudo enviar el HALL a la cola.");
        }
        
        //printf("Estado del sensor de efecto Hall: %d", hall_data.hall_on_off);  

        
        vTaskDelay(pdMS_TO_TICKS(100)); // 
    }
        
}