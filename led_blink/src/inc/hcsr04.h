// Author: Burin Arturo
// Date: 11/04/2025

#define TRIG_PIN GPIO_NUM_4  // Pin TRIG del HC-SR04
#define ECHO_PIN GPIO_NUM_5  // Pin ECHO del HC-SR04
#define LED_PIN GPIO_NUM_2  // Led interno ESP32

void hc_sr04_task(void *pvParameters);