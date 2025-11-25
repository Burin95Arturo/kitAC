// Author: Burin Arturo
// Date: 23/05/2025

// Del pin 6 al 11 se pone en complicado el ESP32, hay que ver bien la funcion del GPIO pero 
// hace que se reinicie el equipo mientras configura.

#define TRIG_PIN            GPIO_NUM_4      // Pin TRIG del HC-SR04
#define ECHO_PIN            GPIO_NUM_5      // Pin ECHO del HC-SR04
#define INTERNAL_LED_PIN    GPIO_NUM_2      // Led interno ESP32
#define HALL_PIN            GPIO_NUM_0      // Pin HALL
#define IR_PIN              GPIO_NUM_32     // Pin IR
#define HX711_DOUT_PIN      GPIO_NUM_18     // Pin de datos (DOUT) del HX711
#define HX711_PD_SCK_PIN    GPIO_NUM_19     // Pin de reloj (PD_SCK) del HX711
#define BUTTON_PESO_PIN     GPIO_NUM_9     // Botón para "Arriba" o "Incrementar"
#define BUTTON_TARA_PIN     GPIO_NUM_12     // Botón para "Abajo" o "Disminuir"
#define BUTTON_ATRAS_PIN    GPIO_NUM_14   // Botón para "Seleccionar" o "Confirmar"
#define BUZZER_PIN          GPIO_NUM_27     // Pin del Buzzer   
#define LED_PIN             GPIO_NUM_39    // Pin del LED externo  
//2da balanza 
#define HX711_2_DOUT_PIN    GPIO_NUM_25     // Pin de datos (DOUT) del HX711 de la 2da balanza
#define HX711_2_PD_SCK_PIN  GPIO_NUM_26     // Pin de reloj (PD_SCK) del HX711 de la 2da balanza

//not tested

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



