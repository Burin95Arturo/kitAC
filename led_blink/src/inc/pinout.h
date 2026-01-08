// Del pin 6 al 11 se pone en complicado el ESP32, hay que ver bien la funcion del GPIO pero 
// hace que se reinicie el equipo mientras configura.

#define TRIG_PIN            GPIO_NUM_33      // Pin TRIG del HC-SR04
#define ECHO_PIN            GPIO_NUM_14      // Pin ECHO del HC-SR04
#define INTERNAL_LED_PIN    GPIO_NUM_2      // Led interno ESP32
#define HALL_PIN            GPIO_NUM_15      // Pin HALL
#define HX711_DOUT_PIN      GPIO_NUM_35     // Pin de datos (DOUT) del HX711
#define HX711_PD_SCK_PIN    GPIO_NUM_26     // Pin de reloj (PD_SCK) del HX711
#define BUTTON_1            GPIO_NUM_15     // Botón para "Arriba" o "Incrementar"
#define BUTTON_2            GPIO_NUM_34     // Botón para "Abajo" o "Disminuir"
#define BUTTON_3            GPIO_NUM_21   // Botón para "Seleccionar" o "Confirmar"
#define BUTTON_4            GPIO_NUM_12   // Botón 4
#define BUZZER_PIN          GPIO_NUM_0     // Pin del Buzzer   
#define LED_PIN             GPIO_NUM_10     // Pin del LED externo  
#define BREAK_PIN           GPIO_NUM_3     // Pin del LED externo  
//2da balanza 
#define HX711_2_DOUT_PIN    GPIO_NUM_39     // Pin de datos (DOUT) del HX711 de la 2da balanza
#define HX711_2_PD_SCK_PIN  GPIO_NUM_25     // Pin de reloj (PD_SCK) del HX711 de la 2da balanza

#define CONFIG_MOSI_GPIO    23
#define CONFIG_SCLK_GPIO    18
#define CONFIG_TFT_CS_GPIO   5

#define CONFIG_DC_GPIO   16 //Data/Command control pin
#define CONFIG_RESET_GPIO  4 //CAMBIÉ ESTO PARA ADECUARLO A LA PLACA DE ARTUR
#define CONFIG_BL_GPIO 17

#define LCD_BK_LIGHT_ON_LEVEL   1

//not tested
