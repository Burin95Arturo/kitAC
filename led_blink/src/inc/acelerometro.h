// Author: Burin Arturo
// Date: 11/12/2025

#define I2C_MASTER_NUM              I2C_NUM_0      // Usaremos el puerto I2C 0
#define I2C_MASTER_FREQ_HZ          100000         // Frecuencia I2C
#define MPU6050_SENSOR_ADDR         0x68           // Dirección I2C del MPU6050 (sin AD0)

// Direcciones de registros importantes del MPU6050
#define MPU6050_ACCEL_XOUT_H    0x3B // Registro de inicio para los 14 bytes de datos
#define MPU6050_PWR_MGMT_1      0x6B // Registro de gestión de energía
#define MPU6050_GYRO_CONFIG     0x1B // Configuración del Giroscopio
#define MPU6050_ACCEL_CONFIG    0x1C // Configuración del Acelerómetro

// Factor de escala (depende de la configuración del sensor, aquí asumimos ±250 °/s y ±2g)
#define GYRO_SCALE_FACTOR       131.0f  // 32768 / 250 (para ±250 deg/s)
#define ACCEL_SCALE_FACTOR      16384.0f // 32768 / 2 (para ±2g)

// Intervalo de tiempo para el filtro (debe coincidir con vTaskDelay)
#define DT                      0.1f // 100 ms (0.1 segundos)

void acelerometro_task(void *pvParameters);