#include "inc/nvs_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "NVS_SYSTEM";
int32_t tara_b1; 
int32_t tara_b2;
int32_t ultimo_peso_medido;

void vNVSTask(void *pvParameters) {
    int32_t contador = 0;
    
    // 1. Intentar leer el valor previo al iniciar
    if (read_nvs_int("ciclos", &contador) == ESP_OK) {
        printf("Valor recuperado de NVS: %ld\n", contador);
    } else {
        printf("No hay datos previos. Iniciando en 0.\n");
    }

    while (1) {
        contador++;
        printf("Guardando nuevo valor: %ld\n", contador);
        
        // 2. Guardar en memoria
        if (write_nvs_int("ciclos", contador) != ESP_OK) {
            printf("Error al escribir en NVS\n");
        }

        // Esperar 10 segundos antes de la siguiente escritura
        // (Es importante no escribir cada milisegundo para cuidar la vida útil de la Flash)
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

esp_err_t init_nvs_storage(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Si la memoria está llena o el formato es distinto, borramos y reintentamos
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t write_nvs_int(const char* key, int32_t value) {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Abrir
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Escribir
    err = nvs_set_i32(my_handle, key, value);
    if (err != ESP_OK) return err;

    // Confirmar cambios
    err = nvs_commit(my_handle);
    
    // Cerrar
    nvs_close(my_handle);
    return err;
}

esp_err_t read_nvs_int(const char* key, int32_t* value) {
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return err;

    err = nvs_get_i32(my_handle, key, value);
    
    nvs_close(my_handle);
    return err;
}