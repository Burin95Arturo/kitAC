#include "inc/buzzer.h"


void buzzer_task(void *pvParameters) {

//    data_t hall_data;
    
    while (1) {

        if (xSemaphoreTake(buzzer_semaphore, portMAX_DELAY) == pdTRUE) {

            // Suena 3 veces el buzzer para avisar el cambio y hace titilar el LED
            gpio_set_level(BUZZER_PIN, 1);
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            
            gpio_set_level(BUZZER_PIN, 0);  
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));

            gpio_set_level(BUZZER_PIN, 1);
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));

            gpio_set_level(BUZZER_PIN, 0);
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));

            gpio_set_level(BUZZER_PIN, 1);
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(BUZZER_PIN, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Leer cada 500 ms
    }
        
}