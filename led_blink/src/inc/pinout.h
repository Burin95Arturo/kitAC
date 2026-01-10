// Del pin 6 al 11 se pone en complicado el ESP32, hay que ver bien la funcion del GPIO pero 
// hace que se reinicie el equipo mientras configura.

// HC-SR04 - Sensor de Altura
#define TRIG_PIN            GPIO_NUM_33      // Pin TRIG del HC-SR04
#define ECHO_PIN            GPIO_NUM_14      // Pin ECHO del HC-SR04

// HX711 N°1 - Balanza 1
#define HX711_DOUT_PIN      GPIO_NUM_35     // Pin de datos (DOUT) del HX711
#define HX711_PD_SCK_PIN    GPIO_NUM_26     // Pin de reloj (PD_SCK) del HX711

// HX711 N°2 - Balanza 2
#define HX711_2_DOUT_PIN    GPIO_NUM_39     // Pin de datos (DOUT) del HX711 de la 2da balanza
#define HX711_2_PD_SCK_PIN  GPIO_NUM_25     // Pin de reloj (PD_SCK) del HX711 de la 2da balanza

// Teclado
#define BUTTON_1            GPIO_NUM_34     // Botón 1 
#define BUTTON_2            GPIO_NUM_27     // Botón 2
#define BUTTON_3            GPIO_NUM_12     // Botón 3
#define BUTTON_4            GPIO_NUM_36     // Botón 4

// Display TFT - ILI9341
#define CONFIG_MOSI_GPIO    23
#define CONFIG_SCLK_GPIO    18
#define CONFIG_TFT_CS_GPIO   5
#define CONFIG_DC_GPIO   16 //Data/Command control pin
#define CONFIG_RESET_GPIO  4 
#define CONFIG_BL_GPIO 17 //Es necesario indicar PIN de Backlight pero no se cablea (va a VDD)
#define LCD_BK_LIGHT_ON_LEVEL   1

// MPU6050 - Acelerómetro para inclinación
#define I2C_MASTER_SCL_IO           32
#define I2C_MASTER_SDA_IO           13

// Freno
#define BREAK_PIN           GPIO_NUM_21     // Pin del LED externo  

// Sensor de efecto Hall - Barandales
#define HALL_PIN            GPIO_NUM_15      // Pin HALL

//Faltan definir estos pines:

// LEDs 
#define LED_PIN             GPIO_NUM_10     // Pin del LED externo
#define INTERNAL_LED_PIN    GPIO_NUM_2      // Led interno ESP32

// Buzzer
#define BUZZER_PIN          GPIO_NUM_0     // Pin del Buzzer 

