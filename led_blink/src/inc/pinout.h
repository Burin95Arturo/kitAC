// Author: Burin Arturo
// Date: 23/05/2025

// Del pin 6 al 11 se pone en complicado el ESP32, hay que ver bien la funcion del GPIO pero 
// hace que se reinicie el equipo mientras configura.

//Sensor altura
#define TRIG_PIN            GPIO_NUM_4      // Pin TRIG del HC-SR04
#define ECHO_PIN            GPIO_NUM_5      // Pin ECHO del HC-SR04

//Led de estado
#define LED_PIN             GPIO_NUM_2      // Led interno ESP32
#define LED_PIN_R             GPIO_NUM_2      // R
#define LED_PIN_G             GPIO_NUM_2      // G
#define LED_PIN_B             GPIO_NUM_2      // B

//Alarma sonora
#define ALARM_PIN             GPIO_NUM_16      // Alarma

//Sensores de baranda
#define HALL_PIN            GPIO_NUM_15      // Pin HALL - Sensor hall (baranda derecha cabecera)
// #define HALL_PIN_2          GPIO_NUM_0      // Pin HALL - Sensor hall (Baranda izquierda cabecera)
// #define HALL_PIN_3          GPIO_NUM_0      // Pin HALL - Sensor hall (Baranda derecha piecera)
// #define HALL_PIN_4          GPIO_NUM_0      // Pin HALL - Sensor hall (Baranda izquierda piecera)

//IR sensor
//#define IR_PIN              GPIO_NUM_32     // Pin IR

//Buttons
#define BUTTON_UP_PIN       GPIO_NUM_25     // Botón para "Arriba" o "Incrementar"
#define BUTTON_DOWN_PIN     GPIO_NUM_26     // Botón para "Abajo" o "Disminuir"
#define BUTTON_SELECT_PIN   GPIO_NUM_27   // Botón para "Seleccionar" o "Confirmar"
#define BUTTON_BACK_PIN     GPIO_NUM_33   // Botón para "Back" o "Cancel"

//1ra balanza
#define HX711_DOUT_PIN      GPIO_NUM_39     // Pin de datos (DOUT) del HX711
#define HX711_PD_SCK_PIN    GPIO_NUM_36     // Pin de reloj (PD_SCK) del HX711

//2da balanza 
#define HX711_2_DOUT_PIN    GPIO_NUM_35     // Pin de datos (DOUT) del HX711 de la 2da balanza
#define HX711_2_PD_SCK_PIN  GPIO_NUM_34     // Pin de reloj (PD_SCK) del HX711 de la 2da balanza

//acelerometro MPU6050

#define I2C_MASTER_SCL_IO   GPIO_NUM_22    // Tu pin SCL (GPIO22)
#define I2C_MASTER_SDA_IO   GPIO_NUM_21    // Tu pin SDA (GPIO21)


// --- Definiciones de Pines para el LCD ---
// ¡AJUSTA ESTOS PINES A TUS CONEXIONES REALES EN EL ESP32!
// Recuerda que estos pines irán conectados al lado de 3.3V del level shifter,
// y el lado de 5V del level shifter se conectará al LCD.
#define LCD_RS_PIN          GPIO_NUM_21  // Register Select (RS)
#define LCD_E_PIN           GPIO_NUM_22  // Enable (E)
#define LCD_DB4_PIN         GPIO_NUM_23  // Data Bit 4 (DB4)
#define LCD_DB5_PIN         GPIO_NUM_17  // Data Bit 5 (DB5)
#define LCD_DB6_PIN         GPIO_NUM_16  // Data Bit 6 (DB6)
#define LCD_DB7_PIN         GPIO_NUM_15   // Data Bit 7 (DB7)

//#define LED_HALL_1          GPIO_NUM_21  // LED HALL 1


