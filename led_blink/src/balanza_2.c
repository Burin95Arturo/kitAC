#include "inc/balanza_2.h"


volatile long hx711_2_raw_reading = 0;
extern long hx711_weight_kg_public;

// --- Variables globales para la lectura (volatile para asegurar que el compilador no optimice lecturas) ---
volatile float hx711_2_weight_kg = 0.0f; // Nueva variable para almacenar el peso en kg

/**
 * @brief Inicializa los pines GPIO para la comunicación con el HX711.
 */
static void hx711_init(void) {
    // Configurar el pin DOUT como entrada
    gpio_reset_pin(HX711_2_DOUT_PIN); // Resetear a estado por defecto
    gpio_set_direction(HX711_2_DOUT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(HX711_2_DOUT_PIN, GPIO_PULLUP_DISABLE); // No se necesita pull-up interno para DOUT

    // Configurar el pin PD_SCK como salida
    gpio_reset_pin(HX711_2_PD_SCK_PIN); // Resetear a estado por defecto
    gpio_set_direction(HX711_2_PD_SCK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HX711_2_PD_SCK_PIN, 0); // Poner PD_SCK en LOW al inicio (modo activo)

    ESP_LOGI(TAG, "HX711: Pines DOUT (GPIO%d) y PD_SCK (GPIO%d) inicializados.", HX711_2_DOUT_PIN, HX711_2_PD_SCK_PIN);
    
    // El HX711 se enciende si PD_SCK está LOW. Si estaba en power-down, tomará un tiempo (aprox 50us) para estabilizarse.
    // Aunque hx711_wait_ready() se encargará de esperar.
}

/**
 * @brief Espera a que el HX711 esté listo para la lectura (DOUT LOW).
 * @param timeout_ticks Tiempo máximo de espera en ticks de FreeRTOS.
 * @return true si el HX711 está listo, false si hay timeout.
 */
static bool hx711_wait_ready(TickType_t timeout_ticks) {
    TickType_t start_time = xTaskGetTickCount();
    while (gpio_get_level(HX711_2_DOUT_PIN) == 1) { // DOUT HIGH significa que no está listo
        if ((xTaskGetTickCount() - start_time) > timeout_ticks) {
            ESP_LOGW(TAG, "HX711: Timeout esperando que DOUT se ponga en LOW.");
            return false; // Timeout
        }
        vTaskDelay(1); // Esperar un tick para ceder CPU y no bloquear completamente
    }
    return true; // HX711 listo (DOUT LOW)
}

/**
 * @brief Lee un valor crudo de 24 bits del HX711.
 * @return El valor leído de 24 bits. Retorna 0 si hay un error (timeout).
 */
static long hx711_read_raw(void) {
    // 1. Esperar a que el HX711 esté listo (DOUT LOW)
    if (!hx711_wait_ready(pdMS_TO_TICKS(200))) { // Esperar hasta 200 ms
        return 0; // Error: HX711 no listo
    }

    // 2. Leer los 24 bits de datos
    long data = 0;
    for (int i = 0; i < 24; i++) {
        gpio_set_level(HX711_2_PD_SCK_PIN, 1); // Generar pulso de reloj (HIGH)
        esp_rom_delay_us(1); // Pequeño delay para asegurar el pulso (datasheet tipicamente 0.2us)
        data = (data << 1) | gpio_get_level(HX711_2_DOUT_PIN); // Leer bit y desplazar
        gpio_set_level(HX711_2_PD_SCK_PIN, 0); // Poner reloj en LOW
        esp_rom_delay_us(1); // Pequeño delay
    }

    // 3. Generar pulsos de reloj para configurar la ganancia y el siguiente canal
    // Esto es crucial para seleccionar el canal (A o B) y la ganancia (128, 64, 32)
    // El número de pulsos después de los 24 bits de datos determina la configuración.
    // 1 pulso: Canal A, Ganancia 128
    // 2 pulsos: Canal B, Ganancia 32
    // 3 pulsos: Canal A, Ganancia 64
    uint8_t pulses_for_config = 0;
#if HX711_GAIN_128
    pulses_for_config = 1; // Canal A, Ganancia 128
#else
    pulses_for_config = 3; // Canal A, Ganancia 64
#endif
    // Para el Canal B (Ganancia 32), serían 2 pulsos.
    // Si quieres usar el Canal B, deberías modificar esta lógica y la configuración del HX711.

    for (int i = 0; i < pulses_for_config; i++) {
        gpio_set_level(HX711_2_PD_SCK_PIN, 1);
        esp_rom_delay_us(1);
        gpio_set_level(HX711_2_PD_SCK_PIN, 0);
        esp_rom_delay_us(1);
    }

    // 4. Manejar el número signado de 24 bits (complemento a 2)
    // Si el bit más significativo (el 23) es 1, el número es negativo.
    if (data & 0x800000) { // Si el bit 23 es 1
        data |= 0xFF000000; // Extender el signo a 32 bits
    }

    return data;
}

void balanza_2_task(void *pvParameters){
    // Inicializar los pines del HX711
    hx711_init();

    // Intentamos leer de la NVS, si falla (porque es la primera vez), quedan en 0
    if (read_nvs_int("tara_b2", &tara_b2) != ESP_OK) tara_b2 = 0;

    long acu_raw_value = 0; 
    central_data_t peso_data_2;
    uint32_t received_request_id; 
    peso_data_2.origen = SENSOR_BALANZA_2;
    printf("Tarea balanza 2 iniciada.\n");
    while (1) {

		/* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación
		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);

        peso_data_2.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido
     
        for (int i = 0; i < HX711_READ_ITERATIONS; i++) {

            acu_raw_value += hx711_read_raw();
            // Esperar 100 ms entre lecturas
            //vTaskDelay(pdMS_TO_TICKS(100));
        }

        peso_data_2.peso_raw1 = acu_raw_value / HX711_READ_ITERATIONS;

        // Enviar la lectura promediada a la cola
        if (xQueueSend(central_queue, &peso_data_2, (TickType_t)0) != pdPASS) {
            // ESP_LOGE(TAG_2, "No se pudo enviar el peso a la cola.");
            printf("No se pudo enviar balanza 2 a la cola.\n");
        }else {
            printf("Raw 2=%ld\n", peso_data_2.peso_raw1);
        }

        // Resetear acumulador para la siguiente lectura
        acu_raw_value = 0;

            //current_raw_value = hx711_read_raw();
            //// 1. Almacenar el valor crudo leído.
            //hx711_raw_reading = current_raw_value; 
            //
            //// 2. Calcular el peso en kilogramos usando los valores de calibración.
            //if (SCALE_FACTOR_VALUE != 0) { // Evitar división por cero
            //    hx711_weight_kg = ( (float)current_raw_value - (float)ZERO_OFFSET_VALUE ) / SCALE_FACTOR_VALUE;
            //} else {
            //    hx711_weight_kg = 0.0f;
            //    //ESP_LOGE(TAG, "ERROR: SCALE_FACTOR_VALUE es cero. Por favor, calibra el sensor.");
            //    printf("ERROR: SCALE_FACTOR_VALUE es cero. Por favor, calibra el sensor.\n");
            //}     
            ////
            //// 3. Imprimir (Enviar) el valor Raw y el Peso en Kg.
                    
            //printf("Lectura Balanza 1: %ld | Peso: %.3f Kg\n", current_raw_value, hx711_weight_kg);

        vTaskDelay(pdMS_TO_TICKS(100)); // Leer cada 100 ms
    }

}