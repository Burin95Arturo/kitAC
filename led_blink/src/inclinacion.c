#include "inc/inclinacion.h"


void inclinacion_task(void *pvParameters) {
	
    _aceleracion_type aceleraciones;
	float angulo_x;
	// float angulo_y;


	/* Buffer de escritura */
	//unsigned char Datos_Tx[]= {MPU6050_WHO_AM_I};
	/* Buffer de lectura */
	//unsigned char Datos_Rx[] = {250};
	/* Check WHO AM I */
	//Chip_I2C_MasterSend (I2C1, MPU6050_I2C_ADDR, Datos_Tx, 1);
	/* Se debe leer 0x68 */
	//Chip_I2C_MasterRead (I2C1, MPU6050_I2C_ADDR, Datos_Rx, 1);

	/* Wakeup MPU6050 */
	MPU_WriteRegister(MPU6050_PWR_MGMT_1,0x00);

	/* Set sample rate to 1kHz */
	MPU_WriteRegister(MPU6050_SMPLRT_DIV,TM_MPU6050_DataRate_1KHz);
    
	//NO CONFIGURO SENSIBILIDAD DEL ACELERÓMETRO. DEFAULT: +/-2G
	//TAMPOCO DEL GIRÓSCOPO
    
	// DatosControl datos_control;
	// datos_control.origen = SENSOR;
    data_t inclinacion_data;


	while(1)
	{
		/* Intenta tomar semaforo del sensor, si no puede se bloquea */
        if (xSemaphoreTake(inclinacion_semaphore, portMAX_DELAY) == pdTRUE) {
	
            MPU_ReadAcceleration(&aceleraciones);

            // angulo_y=atan(aceleraciones.A_X/sqrt(pow(aceleraciones.A_Y,2) + pow(aceleraciones.A_Z,2)))*RAD_TO_DEG;

            angulo_x=atan(aceleraciones.A_Y/sqrt(pow(aceleraciones.A_X,2) + pow(aceleraciones.A_Z,2)))*RAD_TO_DEG;

            /* Sube el dato a la cola */
            inclinacion_data.origen = SENSOR_ACELEROMETRO;
            inclinacion_data.inclinacion = angulo_x;   
            if (xQueueSend(central_queue, &inclinacion_data, (TickType_t)0) != pdPASS) {
                printf( "No se pudo enviar inclinacion a la cola.");
            }
        }

		vTaskDelay(pdMS_TO_TICKS(100));


	}

}



void MPU_WriteRegister(uint8_t REG_ADDRESS, uint8_t value)
{
	static uint8_t tx_data[2];

	tx_data[0]=REG_ADDRESS;
	tx_data[1]=value;

//	Chip_I2C_MasterSend (I2C1, MPU6050_I2C_ADDR, tx_data, 2);
    i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_I2C_ADDR, tx_data, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

}

void MPU_ReadAcceleration(_aceleracion_type* aceleraciones)
{

	uint8_t dir_registro_acel = MPU6050_ACCEL_XOUT_H;
	uint8_t temp[6];


	// Chip_I2C_MasterSend (I2C1, MPU6050_I2C_ADDR, &dir_registro_acel, 1);
	// Chip_I2C_MasterRead (I2C1, MPU6050_I2C_ADDR, temp, 6);
    i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_I2C_ADDR, &dir_registro_acel, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_master_read_from_device(I2C_MASTER_NUM, MPU6050_I2C_ADDR, temp, 6, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

	/* Formatear datos de aceleración leídos*/
	aceleraciones->A_X = (int16_t)(temp[0] << 8 | temp[1]);
	aceleraciones->A_Y = (int16_t)(temp[2] << 8 | temp[3]);
	aceleraciones->A_Z = (int16_t)(temp[4] << 8 | temp[5]);

}
