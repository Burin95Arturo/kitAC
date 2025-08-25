#include "inc/tasktest.h"


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