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
    
void button_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de lectura de botones iniciada.");

    while (1) {
        current_up_state = gpio_get_level(BUTTON_UP_PIN);
        current_down_state = gpio_get_level(BUTTON_DOWN_PIN);
        current_select_state = gpio_get_level(BUTTON_SELECT_PIN);
        
        // Detectar flanco descendente (presión del botón)
        if (current_up_state == false && prev_up_state == true) { // Botón UP presionado
            ESP_LOGI(TAG, "Boton UP presionado!");
        }
        if (current_down_state == false && prev_down_state == true) { // Botón DOWN presionado
            ESP_LOGI(TAG, "Boton DOWN presionado!");
        }
        if (current_select_state == false && prev_select_state == true) { // Botón SELECT presionado
            ESP_LOGI(TAG, "Boton SELECT presionado!");
        }

        // Actualizar estados previos
        prev_up_state = current_up_state;
        prev_down_state = current_down_state;
        prev_select_state = current_select_state;

        vTaskDelay(pdMS_TO_TICKS(50));  // Pequeño delay para debounce de botones
    }
}