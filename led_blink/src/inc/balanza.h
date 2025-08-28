// Author: Burin Arturo
// Date: 10/06/2025

#include "pinout.h"
#include "program.h"

// --- Configuración del HX711 ---
// Ganancia para el Canal A:
// 128 (default) - para la mayoría de las celdas de carga de puente completo.
// 64 - para celdas de carga de medio puente o señales más pequeñas.
// Ganancia para el Canal B: Siempre 32.
#define HX711_GAIN_128 1 // Selecciona la ganancia para el canal A (1 para 128, 0 para 64)

void balanza_task(void *pvParameters);
void balanza_tarar(void);