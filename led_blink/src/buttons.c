#include "inc/buttons.h"


// HACER QUE DEVUELVA "NO_KEY" SI NO SE PRESIONA NINGUN BOTON

void button_task(void *pvParameters) {
    // ESP_LOGI(TAG_BUTTON, "Tarea de lectura de botones iniciada.");
    printf("Tarea de lectura de botones iniciada.\n");
    button_event_t event = EVENT_NO_KEY;
    static bool button_pressed = false;
    central_data_t button_data;
    uint32_t received_request_id; 
    button_data.origen = BUTTON_EVENT;

    while (1) {
        
		/* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación
		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);

        // Leer el estado de los botones
        if (gpio_get_level(BUTTON_1) == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (gpio_get_level(BUTTON_1) == 0) {
                event = EVENT_BUTTON_1;
                button_pressed = true;
            }
        } 
        if (gpio_get_level(BUTTON_2) == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (gpio_get_level(BUTTON_2) == 0) {
                event = EVENT_BUTTON_2;
                button_pressed = true;
            }
        } 
        if (gpio_get_level(BUTTON_3) == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (gpio_get_level(BUTTON_3) == 0) {
                event = EVENT_BUTTON_3;
                button_pressed = true;
            }
        }
        if (gpio_get_level(BUTTON_4) == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (gpio_get_level(BUTTON_4) == 0) {
                event = EVENT_BUTTON_4;
                button_pressed = true;
            }
        }

        if (!button_pressed) {

            event = EVENT_NO_KEY;

        } 
        //printf("Boton: %d\n", event);
        // Siempre enviamos el evento a la cola (mismo si no hubo tecla). Espera 10ms si la cola esta llena.
        button_data.button_event = event;
        button_data.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido
        xQueueSend(central_queue, &button_data, pdMS_TO_TICKS(5));
        // ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
        //printf("Evento de boton enviado a la cola.\n");
        // Retardo para "debounce" del boton, no enviar multiples eventos
        vTaskDelay(pdMS_TO_TICKS(100));
        
        button_pressed = false;
        
    }
}