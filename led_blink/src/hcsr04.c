#include "inc/hcsr04.h"

long getDistance() {
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    long startTime = esp_timer_get_time();
    long endTime = startTime;

    while (gpio_get_level(ECHO_PIN) == 0) {
        startTime = esp_timer_get_time();
    }

    while (gpio_get_level(ECHO_PIN) == 1) {
        endTime = esp_timer_get_time();
    }

    long duration = endTime - startTime;
    long distance = duration / 58;

    return distance;
}

void hc_sr04_task(void *pvParameters) {
  
    long distance;
    float current_height_m = 0.0f;
    central_data_t altura;
    static uint32_t received_request_id; 
    altura.origen = SENSOR_ALTURA;

    while (1) {

        /* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación

		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);
        
        distance = getDistance();
        printf("Distancia: %ld cm\n", distance);

        if (distance < 20) {
            gpio_set_level(INTERNAL_LED_PIN, 1);
        } else {
            gpio_set_level(INTERNAL_LED_PIN, 0);
        }

        current_height_m = (long) distance;
        // Enviar el peso calculado a la cola
        altura.altura = current_height_m;
        altura.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido

        if (xQueueSend(central_queue, &altura, pdMS_TO_TICKS(10)) != pdPASS) {
            // ESP_LOGE(TAG_3, "No se pudo enviar el peso a la cola.");
            printf("No se pudo enviar el peso a la cola.\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
