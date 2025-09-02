#include "inc/buttons.h"


void button_task(void *pvParameters) {
    ESP_LOGI(TAG_BUTTON, "Tarea de lectura de botones iniciada.");
    button_event_t event;
    bool button_pressed = false;
    data_t button_data;

    while (1) {
        
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) {

            // Leer el estado de los botones
            if (gpio_get_level(BUTTON_PESO_PIN) == 0) {
                event = EVENT_BUTTON_PESO;
                button_pressed = true;
            } else if (gpio_get_level(BUTTON_TARA_PIN) == 0) {
                event = EVENT_BUTTON_TARA;
                button_pressed = true;
            } else if (gpio_get_level(BUTTON_ATRAS_PIN) == 0) {
                event = EVENT_BUTTON_ATRAS;
                button_pressed = true;
            }

            // Si se presiono un boton, envia el evento a la cola
            if (button_pressed) {
                // Enviamos el evento a la cola. Espera 10ms si la cola esta llena.
                // Esto es crucial para que la otra tarea se entere del cambio.

                button_data.origen = BUTTON_EVENT;
                button_data.button_event = event;
                xQueueSend(central_queue, &button_data, pdMS_TO_TICKS(10));
                ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
                
                // Retardo para "debounce" del boton, no enviar multiples eventos
                vTaskDelay(pdMS_TO_TICKS(250));
            }

        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño delay para debounce de botones
    }
}