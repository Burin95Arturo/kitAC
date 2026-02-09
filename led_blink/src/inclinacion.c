#include "inc/inclinacion.h"


void inclinacion_task(void *pvParameters) {
	
    
	central_data_t inclinacion_data;
	inclinacion_data.origen = SENSOR_ACELEROMETRO;
	inclinacion_data.inclinacion = 99.99f;

	uint32_t received_request_id; 
	_aceleracion_type aceleraciones;
	float angulo_x;
	float angulo_y;


	/* Buffer de escritura */
	//unsigned char Datos_Tx[]= {MPU6050_WHO_AM_I};
	/* Buffer de lectura */
	//unsigned char Datos_Rx[] = {250};
	/* Check WHO AM I */
	//Chip_I2C_MasterSend (I2C1, MPU6050_I2C_ADDR, Datos_Tx, 1);
	/* Se debe leer 0x68 */
	//Chip_I2C_MasterRead (I2C1, MPU6050_I2C_ADDR, Datos_Rx, 1);

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

	vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo para estabilizar I2C

	/* Wakeup MPU6050 */
	MPU_WriteRegister(MPU6050_PWR_MGMT_1,0x00);

	/* Set sample rate to 1kHz */
	MPU_WriteRegister(MPU6050_SMPLRT_DIV,TM_MPU6050_DataRate_1KHz);
    
	//NO CONFIGURO SENSIBILIDAD DEL ACELERÓMETRO. DEFAULT: +/-2G
	//TAMPOCO DEL GIRÓSCOPO
    

	while(1)
	{
		/* Esperar notificación de Central */
		// Espera indefinidamente (portMAX_DELAY) hasta recibir una notificación
		xTaskNotifyWait(0, 0, &received_request_id, portMAX_DELAY);
		printf("Inclinacion - request_id recibido: %ld\n", received_request_id);
		//printf("Leyendo aceleraciones...\n");
		MPU_ReadAcceleration(&aceleraciones);

		angulo_y=atan(aceleraciones.A_X/sqrt(pow(aceleraciones.A_Y,2) + pow(aceleraciones.A_Z,2)))*RAD_TO_DEG;
		
		angulo_x=atan(aceleraciones.A_Y/sqrt(pow(aceleraciones.A_X,2) + pow(aceleraciones.A_Z,2)))*RAD_TO_DEG;
		
		//printf("A_X: %d, A_Y: %d, A_Z: %d\n", aceleraciones.A_X, aceleraciones.A_Y, aceleraciones.A_Z);
		/* Sube el dato a la cola */
		inclinacion_data.inclinacion = angulo_x;   
//		inclinacion_data.inclinacion = 25.0f; // Valor fijo para pruebas
		inclinacion_data.request_id = received_request_id; // <--- Clave: Devolver el mismo request_id recibido
		
		//(!) Evaluar que pasa si se rompe la comunicación (el calculo daria nan)
		if (inclinacion_data.inclinacion > MAX_ANG_INCLINACION) {
			inclinacion_data.Is_value_an_error = true; // Marca como error si la inclinación es mayor a 90 grados o un angulo negativo (lo que no tendría sentido en este contexto)
			inclinacion_data.inclinacion = 999.0f; // En caso de error, se envia un valor; elijo 999.
		} else {
			inclinacion_data.Is_value_an_error = false;
		}
		
		
		if (xQueueSend(central_queue, &inclinacion_data, pdMS_TO_TICKS(10)) != pdPASS) {
			printf( "No se pudo enviar inclinacion a la cola.");
		}
		else {
			printf("request_id enviado: %ld\n", inclinacion_data.request_id);
		}

		vTaskDelay(pdMS_TO_TICKS(250));


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
