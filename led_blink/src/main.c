// Author: Burin Arturo
// Date: 11/04/2025

/* CON FREERTOS */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"        // Incluye el header de GPIO de ESP-IDF
#include "inc/pinout.h"
#include "esp_log.h"

#include "inc/hcsr04.h"
#include "inc/hall.h"
#include "inc/ir.h"
#include "inc/balanza.h"
#include "inc/buttons.h"
//#include "inc/display_lcd.h"
#include "inc/balanza_2.h"
#include "inc/program.h"
#include "inc/acelerometro.h"
#include <freertos/queue.h>

static const char *TAG = "MSJ";
static const char *TAG_MAIN = "APP_MAIN";
static const char *TAG_GPIO = "GPIO_INIT";

QueueHandle_t button_event_queue = NULL;
QueueHandle_t weight_queue = NULL;
QueueHandle_t height_queue = NULL;
QueueHandle_t error_queue = NULL;

void task_blink(void *pvParameters) {
    while(true) {
        gpio_set_level(LED_PIN, 1);
        //ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        //ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void user_init(void) {
    gpio_config_t io_conf;

    // --- Configuración de Pines de SALIDA ---
    // (LED, TRIG, HX711_PD_SCK)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // NUNCA pull-up/down en salidas
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // NUNCA pull-up/down en salidas

    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (LED) configurado como salida.", LED_PIN);

    // io_conf.pin_bit_mask = (1ULL << LED_HALL_1);
    // gpio_config(&io_conf);
    // ESP_LOGI(TAG_GPIO, "GPIO %d (LED HALL 1) configurado como salida.", LED_HALL_1);

    io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (TRIG) configurado como salida.", TRIG_PIN);

    io_conf.pin_bit_mask = (1ULL << HX711_PD_SCK_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (HX711_PD_SCK) configurado como salida.", HX711_PD_SCK_PIN);

    io_conf.pin_bit_mask = (1ULL << HX711_2_PD_SCK_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (HX711_2_PD_SCK) configurado como salida.", HX711_2_PD_SCK_PIN);

    // --- Configuración de Pines de ENTRADA ---
    // (ECHO, HALL, IR, HX711_DOUT)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Deshabilita interrupciones si no las vas a usar

    // ECHO_PIN (Normalmente push-pull, no necesita pull-up/down interno)
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (ECHO) configurado como entrada.", ECHO_PIN);

    // HALL_PIN (Si es Open-Collector y necesita Pull-Up para estado HIGH)
    io_conf.pin_bit_mask = (1ULL << HALL_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // <-- ¡MUY IMPORTANTE! Si tu sensor es open-collector y requiere pull-up.
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (HALL) configurado como entrada (PULL-UP habilitado).", HALL_PIN);

    // // IR_PIN (Si es Open-Collector y necesita Pull-Up para estado HIGH)
    // io_conf.pin_bit_mask = (1ULL << IR_PIN);
    // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // <-- ¡MUY IMPORTANTE! Si tu sensor es open-collector y requiere pull-up.
    // gpio_config(&io_conf);
    // ESP_LOGI(TAG_GPIO, "GPIO %d (IR) configurado como entrada (PULL-UP habilitado).", IR_PIN);

    // HX711_DOUT_PIN (¡CRÍTICO! Es salida push-pull del HX711, NUNCA habilitar pull-up/down)
    io_conf.pin_bit_mask = (1ULL << HX711_DOUT_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // ¡DESHABILITADO!
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // ¡DESHABILITADO!
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (HX711_DOUT) configurado como entrada (¡SIN PULL-UP/DOWN!).", HX711_DOUT_PIN);
    
    io_conf.pin_bit_mask = (1ULL << BUTTON_UP_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (BUTTON_UP_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_UP_PIN);

    io_conf.pin_bit_mask = (1ULL << BUTTON_DOWN_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (BUTTON_DOWN_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_DOWN_PIN);

    io_conf.pin_bit_mask = (1ULL << BUTTON_SELECT_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (BUTTON_SELECT_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_SELECT_PIN);

    io_conf.pin_bit_mask = (1ULL << BUTTON_BACK_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (BUTTON_BACK_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_BACK_PIN);

    io_conf.pin_bit_mask = (1ULL << HX711_2_DOUT_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // ¡DESHABILITADO!
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // ¡DESHABILITADO!
    gpio_config(&io_conf);
    ESP_LOGI(TAG_GPIO, "GPIO %d (HX711_2_DOUT) configurado como entrada (¡SIN PULL-UP/DOWN!).", HX711_2_DOUT_PIN);

    // COLA DE EVENTOS

    button_event_queue = xQueueCreate(10, sizeof(button_event_t));
    if (button_event_queue == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear la cola de eventos. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    weight_queue = xQueueCreate(1, sizeof(Peso_Data_t));
    if (weight_queue == NULL) {
        ESP_LOGE("MAIN", "No se pudo crear la cola de peso.");
        return;
    }

    height_queue = xQueueCreate(2, sizeof(float));
    if (height_queue == NULL) {
        ESP_LOGE("MAIN", "No se pudo crear la cola de altura.");
        return;
    }

    error_queue = xQueueCreate(1, sizeof(char));
    if (error_queue == NULL) {
        ESP_LOGE("MAIN", "No se pudo crear la cola de peso.");
        return;
    }
    
    // Inicializacion de tareas
    //xTaskCreate(&hc_sr04_task, "hc_sr04_task", 2048, NULL, 1, NULL);
    xTaskCreate(&task_blink, "blink_task", 2048, NULL, 1, NULL);
    //xTaskCreate(&hall_sensor_task, "hall_sensor_task", 2048, NULL, 1, NULL);
    //xTaskCreate(&ir_sensor_task, "ir_sensor_task", 2048, NULL, 1, NULL);
    xTaskCreate(&balanza_task, "balanza_task", 4096, NULL, 1, NULL);
    //xTaskCreate(&button_task, "button_task", 2048, NULL, 1, NULL);     // Pila de 2KB
    //xTaskCreate(&lcd_display_task, "LCD_DisplayTask", 4096, NULL, 5, NULL); // Pila de 4KB
    xTaskCreate(&balanza_2_task, "balanza_2_task", 4096, NULL, 1, NULL);
    //xTaskCreate(&acelerometro_task, "acelerometro", 4096, NULL, 1, NULL);
}

void app_main(void)
{
    esp_log_level_set(TAG_MAIN, ESP_LOG_INFO);
    esp_log_level_set(TAG_GPIO, ESP_LOG_INFO);

    ESP_LOGI(TAG_MAIN, "Iniciando app_main()...");
    user_init();
    
    ESP_LOGI(TAG_MAIN, "app_main() ha finalizado. Las tareas de FreeRTOS estan ahora ejecutandose.");
}

/* SIN FREERTOS */

// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_log.h"

// #define LED_PIN GPIO_NUM_2

// static const char *TAG = "MSJ";

// void app_main(void) {
//     gpio_config_t io_conf;
//     io_conf.intr_type = GPIO_INTR_DISABLE;
//     io_conf.mode = GPIO_MODE_OUTPUT;
//     io_conf.pin_bit_mask = (1ULL << LED_PIN);
//     gpio_config(&io_conf);

//     while (1) {
//         gpio_set_level(LED_PIN, 1);
//         ESP_LOGI(TAG, "LED ON");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         gpio_set_level(LED_PIN, 0);
//         ESP_LOGI(TAG, "LED OFF");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }