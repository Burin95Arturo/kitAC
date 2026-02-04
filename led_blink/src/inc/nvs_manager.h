// Author: Arturo Burin  
// Date: 22/01/2026

#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define STORAGE_NAMESPACE "storage"

// Definicion de la task
void vNVSTask(void *pvParameters);

// Inicializa el almacenamiento NVS
esp_err_t init_nvs_storage(void);

// Guarda un entero de 32 bits
esp_err_t write_nvs_int(const char* key, int32_t value);

// Lee un entero de 32 bits
esp_err_t read_nvs_int(const char* key, int32_t* value);

#endif