
#ifndef MPU6050_H_
#define MPU6050_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "esp_timer.h" // Incluye esp_timer para medir el tiempo
#include "inc/pinout.h"
#include "inc/program.h"
#include <freertos/queue.h>
#include "driver/i2c.h"
#include "math.h"

#define RAD_TO_DEG (180.0/3.14) //Radianes a grados: *180/pi


/* Default I2C address */
#define MPU6050_I2C_ADDR			0x68

/* Who I am register value */
#define MPU6050_I_AM				0x68

/* MPU6050 registers */
#define MPU6050_AUX_VDDIO			0x01
#define MPU6050_SMPLRT_DIV			0x19
#define MPU6050_CONFIG				0x1A
#define MPU6050_GYRO_CONFIG			0x1B
#define MPU6050_ACCEL_CONFIG		0x1C
#define MPU6050_MOTION_THRESH		0x1F
#define MPU6050_INT_PIN_CFG			0x37
#define MPU6050_INT_ENABLE			0x38
#define MPU6050_INT_STATUS			0x3A
#define MPU6050_ACCEL_XOUT_H		0x3B
#define MPU6050_ACCEL_XOUT_L		0x3C
#define MPU6050_ACCEL_YOUT_H		0x3D
#define MPU6050_ACCEL_YOUT_L		0x3E
#define MPU6050_ACCEL_ZOUT_H		0x3F
#define MPU6050_ACCEL_ZOUT_L		0x40
#define MPU6050_TEMP_OUT_H			0x41
#define MPU6050_TEMP_OUT_L			0x42
#define MPU6050_GYRO_XOUT_H			0x43
#define MPU6050_GYRO_XOUT_L			0x44
#define MPU6050_GYRO_YOUT_H			0x45
#define MPU6050_GYRO_YOUT_L			0x46
#define MPU6050_GYRO_ZOUT_H			0x47
#define MPU6050_GYRO_ZOUT_L			0x48
#define MPU6050_MOT_DETECT_STATUS	0x61
#define MPU6050_SIGNAL_PATH_RESET	0x68
#define MPU6050_MOT_DETECT_CTRL		0x69
#define MPU6050_USER_CTRL			0x6A
#define MPU6050_PWR_MGMT_1			0x6B
#define MPU6050_PWR_MGMT_2			0x6C
#define MPU6050_FIFO_COUNTH			0x72
#define MPU6050_FIFO_COUNTL			0x73
#define MPU6050_FIFO_R_W			0x74
#define MPU6050_WHO_AM_I			0x75

/* Gyro sensitivities in degrees/s */
#define MPU6050_GYRO_SENS_250		((float) 131)
#define MPU6050_GYRO_SENS_500		((float) 65.5)
#define MPU6050_GYRO_SENS_1000		((float) 32.8)
#define MPU6050_GYRO_SENS_2000		((float) 16.4)

/* Acce sensitivities in g/s */
#define MPU6050_ACCE_SENS_2			((float) 16384)
#define MPU6050_ACCE_SENS_4			((float) 8192)
#define MPU6050_ACCE_SENS_8			((float) 4096)
#define MPU6050_ACCE_SENS_16		((float) 2048)


#define TM_MPU6050_DataRate_8KHz       0   /*!< Sample rate set to 8 kHz */
#define TM_MPU6050_DataRate_4KHz       1   /*!< Sample rate set to 4 kHz */
#define TM_MPU6050_DataRate_2KHz       3   /*!< Sample rate set to 2 kHz */
#define TM_MPU6050_DataRate_1KHz       7   /*!< Sample rate set to 1 kHz */
#define TM_MPU6050_DataRate_500Hz      15  /*!< Sample rate set to 500 Hz */
#define TM_MPU6050_DataRate_250Hz      31  /*!< Sample rate set to 250 Hz */
#define TM_MPU6050_DataRate_125Hz      63  /*!< Sample rate set to 125 Hz */
#define TM_MPU6050_DataRate_100Hz      79  /*!< Sample rate set to 100 Hz */


#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_SCL_IO           36
#define I2C_MASTER_SDA_IO           33
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TIMEOUT_MS       1000


typedef struct _aceleracion_type {

	int16_t A_X; /*!< Accelerometer value X axis */
	int16_t A_Y; /*!< Accelerometer value Y axis */
	int16_t A_Z; /*!< Accelerometer value Z axis */

} _aceleracion_type;

void inclinacion_task(void *pvParameters);
void MPU_WriteRegister(uint8_t REG_ADDRESS, uint8_t value);
void MPU_ReadAcceleration(_aceleracion_type* aceleraciones);

#endif /* MPU6050_H_ */