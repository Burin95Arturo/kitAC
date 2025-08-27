// Author: Burin Arturo
// Date: 01/08/2025

#ifndef __PROGRAM__
#define __PROGRAM__


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "freertos/semphr.h"

typedef enum {
    EVENT_BUTTON_UP,
    EVENT_BUTTON_DOWN,
    EVENT_BUTTON_SELECT,
} button_event_t;

extern QueueHandle_t button_event_queue;
extern QueueHandle_t weight_queue;       // Cola para los datos de peso (float)
extern QueueHandle_t height_queue;       // Cola para los datos de altura (float)
extern QueueHandle_t central_queue;
extern SemaphoreHandle_t acelerometro_semaphore; // Semáforo binario para el acelerómetro


typedef enum {
    SENSOR_ALTURA=0,
    SENSOR_HALL,
    SENSOR_IR,
    SENSOR_BALANZA,
    TEST_TASK
} sensor_origen_t;

typedef struct {
    sensor_origen_t origen;
    long altura;
    float peso;
    bool on_off;
} data_t;

static const char *TAG;

#endif
