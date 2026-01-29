#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"        // Incluye el header de GPIO de ESP-IDF
#include "driver/spi_master.h"
#include "inc/pinout.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "inc/hcsr04.h"
#include "inc/hall.h"
#include "inc/ir.h"
#include "inc/balanza.h"
#include "inc/buttons.h"
#include "inc/display_tft.h"
#include "inc/balanza_2.h"
#include "inc/program.h"
#include "inc/inclinacion.h"
#include "inc/nvs_manager.h"
// Definiciones de sensor_origen_t y data_t se moverán a program.h

#include "inc/tasktest.h"
#include "inc/central.h"
#include "inc/buzzer.h"
#include "inc/break.h"
#include <freertos/queue.h>

static const char *TAG_MSJ = "MSJ";
static const char *TAG_MAIN = "APP_MAIN";
static const char *TAG_GPIO = "GPIO_INIT";

#define MAX_LENGHT_QUEUE 10

QueueHandle_t central_queue = NULL;
QueueHandle_t display_queue = NULL;

// Handles de semáforos binarios
SemaphoreHandle_t task_test_semaphore = NULL;
SemaphoreHandle_t peso_semaphore = NULL;
SemaphoreHandle_t peso_semaphore_2 = NULL;
SemaphoreHandle_t hall_semaphore = NULL;
SemaphoreHandle_t ir_semaphore = NULL;
SemaphoreHandle_t altura_semaphore = NULL;
SemaphoreHandle_t button_semaphore = NULL;
SemaphoreHandle_t inclinacion_semaphore = NULL;
SemaphoreHandle_t buzzer_semaphore = NULL;
SemaphoreHandle_t break_semaphore = NULL;

// Handle de tareas de sensores (necesario para Notify)
TaskHandle_t inclinacion_task_handle = NULL;
TaskHandle_t balanza_task_handle = NULL;
TaskHandle_t balanza_2_task_handle = NULL;
TaskHandle_t barandales_task_handle = NULL;
TaskHandle_t altura_task_handle = NULL;
TaskHandle_t freno_task_handle = NULL;
TaskHandle_t teclado_task_handle = NULL;



void task_blink(void *pvParameters) {
    while(true) {
        gpio_set_level(INTERNAL_LED_PIN, 1);
        ESP_LOGI(TAG_MSJ, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(INTERNAL_LED_PIN, 0);
        ESP_LOGI(TAG_MSJ, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void user_init(void) {
    gpio_config_t io_conf;

    /*DESCOMENTAR INICIALIZACIÓN DE PINES*/

    
    // --- Configuración de Pines de SALIDA ---
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // NUNCA pull-up/down en salidas
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // NUNCA pull-up/down en salidas

    io_conf.pin_bit_mask = (1ULL << INTERNAL_LED_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (LED) configurado como salida.", INTERNAL_LED_PIN);

    io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (TRIG) configurado como salida.", TRIG_PIN);

    io_conf.pin_bit_mask = (1ULL << HX711_PD_SCK_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (HX711_PD_SCK) configurado como salida.", HX711_PD_SCK_PIN);

    io_conf.pin_bit_mask = (1ULL << HX711_2_PD_SCK_PIN);
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (HX711_2_PD_SCK) configurado como salida.", HX711_2_PD_SCK_PIN);

    // --- Configuración de Pines de ENTRADA ---
    // (ECHO, HALL, IR, HX711_DOUT)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Deshabilita interrupciones si no las vas a usar

    // ECHO_PIN (Normalmente push-pull, no necesita pull-up/down interno)
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (ECHO) configurado como entrada.", ECHO_PIN);

    // HALL_PIN (Si es Open-Collector y necesita Pull-Up para estado HIGH)
    io_conf.pin_bit_mask = (1ULL << HALL_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // <-- ¡MUY IMPORTANTE! Si tu sensor es open-collector y requiere pull-up.
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (HALL) configurado como entrada (PULL-UP habilitado).", HALL_PIN);




    // HX711_DOUT_PIN (¡CRÍTICO! Es salida push-pull del HX711, NUNCA habilitar pull-up/down)
    io_conf.pin_bit_mask = (1ULL << HX711_DOUT_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // ¡DESHABILITADO!
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // ¡DESHABILITADO!
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (HX711_DOUT) configurado como entrada (¡SIN PULL-UP/DOWN!).", HX711_DOUT_PIN);
    
    io_conf.pin_bit_mask = (1ULL << BUTTON_1);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (BUTTON_UP_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_1);

    io_conf.pin_bit_mask = (1ULL << BUTTON_2);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (BUTTON_DOWN_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_2);

    io_conf.pin_bit_mask = (1ULL << BUTTON_3);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (BUTTON_SELECT_PIN) configurado como entrada.(PULL-UP habilitado)", BUTTON_3);

    io_conf.pin_bit_mask = (1ULL << BUTTON_4);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (BUTTON_4) configurado como entrada.(PULL-UP habilitado)", BUTTON_4);

    io_conf.pin_bit_mask = (1ULL << HX711_2_DOUT_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // ¡DESHABILITADO!
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // ¡DESHABILITADO!
    gpio_config(&io_conf);
    ESP_LOGI(TAG_MSJ, "GPIO %d (HX711_2_DOUT) configurado como entrada (¡SIN PULL-UP/DOWN!).", HX711_2_DOUT_PIN);

    // // Crear el semáforo binario para acelerómetro
    // task_test_semaphore = xSemaphoreCreateBinary();
    // if (task_test_semaphore == NULL) {
    //     ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario acelerometro_semaphore. Reiniciando...");
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     esp_restart();
    // }

    peso_semaphore = xSemaphoreCreateBinary();
    if (peso_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario peso_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    peso_semaphore_2 = xSemaphoreCreateBinary();
    if (peso_semaphore_2 == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario peso_semaphore_2. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    hall_semaphore = xSemaphoreCreateBinary();
    if (hall_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario hall_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    ir_semaphore = xSemaphoreCreateBinary();
    if (ir_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario ir_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    altura_semaphore = xSemaphoreCreateBinary();
    if (altura_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario ir_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    button_semaphore = xSemaphoreCreateBinary();
    if (button_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario button_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    inclinacion_semaphore = xSemaphoreCreateBinary();
    if (inclinacion_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario inclinacion_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    buzzer_semaphore = xSemaphoreCreateBinary();
    if (buzzer_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario buzzer_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    break_semaphore = xSemaphoreCreateBinary();
    if (break_semaphore == NULL) {
        ESP_LOGE(TAG_MAIN, "Fallo al crear el semáforo binario break_semaphore. Reiniciando...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }

    central_queue = xQueueCreate(MAX_LENGHT_QUEUE, sizeof(central_data_t));
    if (central_queue == NULL) {
        ESP_LOGE("MAIN", "No se pudo crear la cola central.");
        return;
    }
    display_queue = xQueueCreate(MAX_LENGHT_QUEUE, sizeof(display_t));
    if (display_queue == NULL) {
        ESP_LOGE("MAIN", "No se pudo crear la cola del display.");
        return;
    }

    // Inicializacion de tareas
    //Las tabuladas ya estaban comentadas

    xTaskCreate(&hc_sr04_task, "hc_sr04_task", 2048, NULL, 1, &altura_task_handle);
            //xTaskCreate(&task_blink, "blink_task", 2048, NULL, 1, NULL);
    xTaskCreate(&hall_sensor_task, "hall_sensor_task", 2048, NULL, 1, &barandales_task_handle);
    //xTaskCreate(&ir_sensor_task, "ir_sensor_task", 2048, NULL, 1, NULL);
    xTaskCreate(&balanza_task, "balanza_task", 4096, NULL, 1, &balanza_task_handle);
    xTaskCreate(&button_task, "button_task", 2048, NULL, 1, &teclado_task_handle);     // Pila de 2KB
    xTaskCreate(&balanza_2_task, "balanza_2_task", 4096, NULL, 1, &balanza_2_task_handle);
            //xTaskCreate(&tasktest, "test_task", 2048, NULL, 1, NULL);     // Pila de 2K
    //xTaskCreate(&nuevo_central, "nuevo_central_task", 2048, NULL, 1, NULL); // Pila de 2K
    //xTaskCreate(&buzzer_task, "buzzer_task", 2048, NULL, 1, NULL);
    xTaskCreate(&inclinacion_task, "inclinacion_task", 4096, NULL, 1, &inclinacion_task_handle);
    xTaskCreate(&display_tft_task, "tft_task", 24576, NULL, 1, NULL); // Pila de 24kb
    // xTaskCreate(&simulation_task, "simu_task", 2048, NULL, 1, NULL);
    xTaskCreate(&vNVSTask, "nvs_task", 4096, NULL, 5, NULL);

    xTaskCreate(&break_task, "break_task", 2048, NULL, 1, &freno_task_handle);

}

void app_main(void)
{
    esp_log_level_set(TAG_MAIN, ESP_LOG_INFO);
    esp_log_level_set(TAG_GPIO, ESP_LOG_INFO);

    // Inicializar NVS al arrancar
    ESP_ERROR_CHECK(init_nvs_storage());

    ESP_LOGI(TAG_MAIN, "Iniciando app_main()...");
    user_init();
    
    // ¡IMPORTANTE! app_main() debe terminar su ejecución aquí.
    // FreeRTOS se encargará de ejecutar las tareas que has creado.
    // NO PONER UN BUCLE while(true) VACÍO AQUÍ.
    ESP_LOGI(TAG_MAIN, "app_main() ha finalizado. Las tareas de FreeRTOS estan ahora ejecutandose.");

}