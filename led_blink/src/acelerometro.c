// Author: Burin Arturo
// Date: 11/12/2025

#include "inc/acelerometro.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h"
#include "inc/pinout.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>


static const char *TAG = "ACELEROMETRO_TASK";
static const char *TAG_REAL = "MPU6050";

// Variables estáticas para mantener el estado del filtro entre llamadas
static float current_roll = 0.0f;
static float current_pitch = 0.0f;
// El yaw (cabeceo) es difícil de calcular sin un magnetómetro (MPU9250 o similar)
// pero se puede estimar por el giro.
static float current_yaw = 0.0f;


void acelerometro_init() {
    // 1. Configuración de los pines I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    ESP_LOGI(TAG, "Acelerómetro (I2C) inicializado en SDA: %d, SCL: %d", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    
    // 2. Inicialización del MPU6050 (Despertarlo)
    uint8_t power_management[2] = {MPU6050_PWR_MGMT_1, 0x00}; // Despierta al sensor
    
    esp_err_t err = i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        MPU6050_SENSOR_ADDR, 
        power_management, 
        2, 
        1000 / portTICK_PERIOD_MS
    );
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MPU6050 Despertado y listo.");
    } else {
        ESP_LOGE(TAG, "FALLÓ al inicializar el MPU6050. Código: %d", err);
    }
}

// --- Función I2C Auxiliar (Debe estar implementada en tu proyecto) ---

/**
 * @brief Lee N bytes desde el registro 'reg_addr' del MPU6050.
 */
esp_err_t mpu6050_read_bytes(uint8_t reg_addr, uint8_t *data, size_t len) {
    // Implementación del driver I2C de ESP-IDF para leer
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_SENSOR_ADDR, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
    // return ESP_OK; // Asumimos éxito por simplicidad
}

// --- Implementación de la Lectura Real ---

void acelerometro_leer_angulos(float *roll, float *pitch, float *yaw) {
    uint8_t data[14] = {0}; // 6 Accel, 6 Gyro, 2 Temp (Total 14 bytes)
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx_raw, gy_raw, gz_raw;
    float accel_roll, accel_pitch;
    float gyro_x, gyro_y, gyro_z;

    // Paso 1: Leer los 14 bytes de datos crudos del MPU6050
    if (mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, data, 14) != ESP_OK) {
        ESP_LOGE(TAG_REAL, "Error al leer datos del MPU6050");
        return; // Salir si hay error de I2C
    }

    // Desplazamiento de los datos de 8 bits a 16 bits (High byte + Low byte)
    ax_raw = (int16_t)((data[0] << 8) | data[1]);
    ay_raw = (int16_t)((data[2] << 8) | data[3]);
    az_raw = (int16_t)((data[4] << 8) | data[5]);
    // (Omitimos la temperatura)
    gx_raw = (int16_t)((data[8] << 8) | data[9]);
    gy_raw = (int16_t)((data[10] << 8) | data[11]);
    gz_raw = (int16_t)((data[12] << 8) | data[13]);

    // Paso 2: Conversión a unidades físicas (grados/s y g's)
    gyro_x = (float)gx_raw / GYRO_SCALE_FACTOR;
    gyro_y = (float)gy_raw / GYRO_SCALE_FACTOR;
    gyro_z = (float)gz_raw / GYRO_SCALE_FACTOR;

    // Paso 3: Cálculo del ángulo inicial (Acelerómetro)
    // El acelerómetro da ángulos con ruido pero estables a largo plazo
    // Usamos atan2 para evitar divisiones por cero y tener el rango completo ±180°
    
    // Cálculo de Roll (inclinación lateral, rotación alrededor del eje X)
    accel_roll = atan2f((float)ay_raw, sqrtf( (float)ax_raw * (float)ax_raw + (float)az_raw * (float)az_raw)) * 180.0f / M_PI;

    // Cálculo de Pitch (inclinación frontal/trasera, rotación alrededor del eje Y)
    accel_pitch = atan2f((float)ax_raw, sqrtf( (float)ay_raw * (float)ay_raw + (float)az_raw * (float)az_raw)) * 180.0f / M_PI;
    
    // Paso 4: Fusión de Sensores (Filtro Complementario simple)
    // Combina la estabilidad a largo plazo del acelerómetro con la respuesta rápida del giroscopio
    
    // a) El ángulo del giroscopio se calcula integrando la velocidad angular
    //    (Velocidad angular en X * Delta_Tiempo) -> cambio de Roll
    current_roll = 0.98f * (current_roll + gyro_x * DT) + 0.02f * accel_roll;

    // b) El ángulo del giroscopio se calcula integrando la velocidad angular
    //    (Velocidad angular en Y * Delta_Tiempo) -> cambio de Pitch
    current_pitch = 0.98f * (current_pitch + gyro_y * DT) + 0.02f * accel_pitch;

    // c) Yaw se calcula solo por integración del giroscopio (drift)
    current_yaw += gyro_z * DT;

    // Paso 5: Asignar los valores calculados
    *roll = current_roll;
    *pitch = current_pitch;
    *yaw = current_yaw; 
    // printf("Angulos | Roll (X): %7.2f° | Pitch (Y): %7.2f° | Yaw (Z): %7.2f°\n", 
    //            current_roll, current_pitch, current_yaw);
}

// --- Tarea de FreeRTOS ---

void acelerometro_task(void *pvParameters){
    float roll, pitch, yaw;
    
    // Inicializar el hardware I2C antes de entrar al bucle
    acelerometro_init();
    
    while (1) {
        // 1. Leer los ángulos
        acelerometro_leer_angulos(&roll, &pitch, &yaw);
        
        // 2. Imprimir los valores en la consola
        printf("Angulos | Roll (X): %7.2f° | Pitch (Y): %7.2f° | Yaw (Z): %7.2f°\n", 
               roll, pitch, yaw);
        
        // 3. Esperar un tiempo (100ms) antes de la próxima lectura
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}