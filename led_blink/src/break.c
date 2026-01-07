#include "inc/break.h"


void break_task(void *pvParameters) {
    printf("Tarea de lectura del freno iniciada.\n");
    bool button_pressed = false;
    static uint32_t received_request_id; 
    central_data_t break_data;                
    break_data.origen = SENSOR_FRENO;


    while (1) {
        
        /* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación

		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);

        // Leer el estado de los botones
        if (gpio_get_level(BREAK_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BREAK_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));        
                button_pressed = true;
            }
        }

        break_data.freno_on_off = button_pressed;
        break_data.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido
        xQueueSend(central_queue, &break_data, pdMS_TO_TICKS(10));
        // ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
        printf("Evento de freno enviado a la cola.\n");
        // Retardo para "debounce" del boton, no enviar multiples eventos
        vTaskDelay(pdMS_TO_TICKS(250));
        button_pressed = false;
    }
}