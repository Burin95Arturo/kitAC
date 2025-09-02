#include "pinout.h"
#include "program.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "driver/gpio.h"
#include "esp_log.h"

// --- Configuración del HX711 ---
// Ganancia para el Canal A:
// 128 (default) - para la mayoría de las celdas de carga de puente completo.
// 64 - para celdas de carga de medio puente o señales más pequeñas.
// Ganancia para el Canal B: Siempre 32.
#define HX711_GAIN_128 1 // Selecciona la ganancia para el canal A (1 para 128, 0 para 64)
// --- Prototipos de funciones ---
static void hx711_init(void);
static bool hx711_wait_ready(TickType_t timeout_ticks);
static long hx711_read_raw(void);

// --- Tag para el logging del ESP-IDF ---
static const char *TAG_2 = "HX711_DRIVER";

// --- Constantes de Calibración (¡AJUSTA ESTOS VALORES DESPUÉS DE CALIBRAR FÍSICAMENTE!) ---
// Este es el valor crudo del HX711 cuando no hay peso sobre la celda de carga.
// Obtén este valor promediando varias lecturas sin carga.
#define ZERO_OFFSET_VALUE 82600L // Raw value obtenido con balanza sin carga (promediado 10)

// Este es el factor de escala: cuántos "tics" crudos del HX711 equivalen a 1 kilogramo.
// Calcula: (Lectura_con_Peso - ZERO_OFFSET_VALUE) / Peso_Conocido_en_Kg
#define SCALE_FACTOR_VALUE 26847.8260f // Se uso una pesa de 4.6Kg y una balanza de presicion


void balanza_task(void *pvParameters);