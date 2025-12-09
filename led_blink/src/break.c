#include "inc/break.h"


void break_task(void *pvParameters) {
    printf("Tarea de lectura del freno iniciada.\n");
    bool button_pressed = false;
    data_t break_data;

    while (1) {
        
        if (xSemaphoreTake(break_semaphore, portMAX_DELAY) == pdTRUE) {

            // Leer el estado de los botones
            if (gpio_get_level(BREAK_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(50));
                if (gpio_get_level(BREAK_PIN) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(50));        
                    button_pressed = true;
                }
            }
            if (button_pressed) {
                break_data.origen = SENSOR_FRENO;
                break_data.freno_on_off = true;
                xQueueSend(central_queue, &break_data, pdMS_TO_TICKS(10));
                // ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
                printf("Evento de freno enviado a la cola.\n");
                // Retardo para "debounce" del boton, no enviar multiples eventos
                vTaskDelay(pdMS_TO_TICKS(250));
            }

        }
        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño delay para debounce de botones
    }
}