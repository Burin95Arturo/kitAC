// Author: Burin Arturo
// Date: 25/05/2025

#include "inc/ir.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"

static bool getIrSensorState(void);

static bool getIrSensorState(void){
    
    return gpio_get_level(IR_PIN);
}

void ir_sensor_task(void *pvParameters) {

    while (1) {
        
        //esp_rom_delay_us(10);

        gpio_set_level(LED_PIN, getIrSensorState());
        
       //vTaskDelay(pdMS_TO_TICKS(100));
    }
}