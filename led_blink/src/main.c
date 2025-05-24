// Author: Burin Arturo
// Date: 11/04/2025

/* CON FREERTOS */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"        // Incluye el header de GPIO de ESP-IDF
#include "inc/hcsr04.h"
#include "inc/hall.h"
#include "inc/pinout.h"



void task_blink(void *pvParameters) {
    while(true) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void user_init(void) {
    // Inicialización del usuario
    gpio_config_t io_conf; // Usa gpio_config_t

    io_conf.intr_type = GPIO_INTR_DISABLE; // Usa GPIO_INTR_DISABLE
    io_conf.mode = GPIO_MODE_OUTPUT; // Usa GPIO_MODE_OUTPUT
    io_conf.pin_bit_mask = (1ULL << LED_PIN); // Define la máscara de bits del pin
    io_conf.pull_down_en = 0; // Sin pull-down
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Usa GPIO_PULLUP_ENABLE
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << HALL_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLDOWN_ENABLE; // Sino solo se apaga con masa y se la pasa prendido
    gpio_config(&io_conf);

    // Inicializacion de tareas
    //xTaskCreate(&hc_sr04_task, "hc_sr04_task", 2048, NULL, 1, NULL);
    //xTaskCreate(&task_blink, "blink_task", 2048, NULL, 1, NULL);
    xTaskCreate(&hall_sensor_task, "hall_sensor_task", 2048, NULL, 1, NULL);
}

void app_main(void)
{
    user_init();
    while(true)
    {
        // vTaskDelay(portTICK_PERIOD_MS); // Evita el watchdog
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* SIN FREERTOS */

// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"

// #define LED_PIN GPIO_NUM_2

// void app_main(void) {
//     gpio_config_t io_conf;
//     io_conf.intr_type = GPIO_INTR_DISABLE;
//     io_conf.mode = GPIO_MODE_OUTPUT;
//     io_conf.pin_bit_mask = (1ULL << LED_PIN);
//     gpio_config(&io_conf);

//     while (1) {
//         gpio_set_level(LED_PIN, 1);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         gpio_set_level(LED_PIN, 0);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }