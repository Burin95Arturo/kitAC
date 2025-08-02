// Author: Burin Arturo
// Date: 19/06/2025

#include "inc/buttons.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "esp_log.h"

static const char *TAG = "BUTTONS";
bool prev_up_state          = true;     // true = botón no presionado (pull-up)
bool prev_down_state        = true;
bool prev_select_state      = true;
bool current_up_state       = true;
bool current_down_state     = true;
bool current_select_state   = true;
    
// Tags para logs
static const char *TAG_BTN = "BUTTON_TASK";

void button_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de lectura de botones iniciada.");

    while (1) {
        button_event_t event;
        bool button_pressed = false;

        // Leer el estado de los botones
        if (gpio_get_level(BUTTON_UP_PIN) == 0) {
            event = EVENT_BUTTON_UP;
            button_pressed = true;
        } else if (gpio_get_level(BUTTON_DOWN_PIN) == 0) {
            event = EVENT_BUTTON_DOWN;
            button_pressed = true;
        } else if (gpio_get_level(BUTTON_SELECT_PIN) == 0) {
            event = EVENT_BUTTON_SELECT;
            button_pressed = true;
        }

        // Si se presiono un boton, envia el evento a la cola
        if (button_pressed) {
            // Enviamos el evento a la cola. Espera 10ms si la cola esta llena.
            // Esto es crucial para que la otra tarea se entere del cambio.
            xQueueSend(button_event_queue, &event, pdMS_TO_TICKS(10));
            ESP_LOGD(TAG_BTN, "Evento de boton enviado a la cola.");
            
            // Retardo para "debounce" del boton, no enviar multiples eventos
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño delay para debounce de botones
    }
}