// Author: Burin Arturo
// Date: 23/05/2025

#include "inc/hall.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "esp_log.h"

static const char *TAG = "MSJ";
static uint8_t contador_barandas = 0;


static uint8_t getHallSensorState(void);

static uint8_t getHallSensorState(void){
    contador_barandas = 0;
    contador_barandas += gpio_get_level(HALL_PIN);
    contador_barandas += gpio_get_level(HALL_PIN_2);
    contador_barandas += gpio_get_level(HALL_PIN_3);
    contador_barandas += gpio_get_level(HALL_PIN_4);
    return contador_barandas;
}

void hall_sensor_task(void *pvParameters) {
    uint8_t hall_state = 0;
    
    while (1) {
        
        //esp_rom_delay_us(10);
        hall_state = getHallSensorState();
        //gpio_set_level(LED_HALL_1, getHallSensorState());
        ESP_LOGI(TAG, "Estado de barandas: %d", hall_state);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}