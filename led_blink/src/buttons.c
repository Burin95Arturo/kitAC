#include "inc/buttons.h"


// HACER QUE DEVUELVA "NO_KEY" SI NO SE PRESIONA NINGUN BOTON

void button_task(void *pvParameters) {
    // ESP_LOGI(TAG_BUTTON, "Tarea de lectura de botones iniciada.");
    printf("Tarea de lectura de botones iniciada.\n");
    button_event_t event;
    bool button_pressed = false;
    data_t button_data;

    while (1) {
        
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) {

            // Leer el estado de los botones
            if (gpio_get_level(BUTTON_1) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(BUTTON_1) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(50));        
                    event = EVENT_BUTTON_1;
                    button_pressed = true;
                }
            } else if (gpio_get_level(BUTTON_2) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(BUTTON_2) == 0) {
                    event = EVENT_BUTTON_2;
                    button_pressed = true;
                }
            } else if (gpio_get_level(BUTTON_3) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(BUTTON_3) == 0) {
                    event = EVENT_BUTTON_3;
                    button_pressed = true;
                }
            }
            else if (gpio_get_level(BUTTON_4) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(BUTTON_4) == 0) {
                    event = EVENT_BUTTON_4;
                    button_pressed = true;
                }
            }
            // Si se presiono un boton, envia el evento a la cola
            if (button_pressed) {
                // Enviamos el evento a la cola. Espera 10ms si la cola esta llena.
                // Esto es crucial para que la otra tarea se entere del cambio.

                button_data.origen = BUTTON_EVENT;
                button_data.button_event = event;
                xQueueSend(central_queue, &button_data, pdMS_TO_TICKS(10));
                // ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
                printf("Evento de boton enviado a la cola.\n");
                // Retardo para "debounce" del boton, no enviar multiples eventos
                vTaskDelay(pdMS_TO_TICKS(250));
            }

        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño delay para debounce de botones
    }
}