#ifndef __PROGRAM__
#define __PROGRAM__


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "freertos/semphr.h"

typedef enum {
    EVENT_BUTTON_PESO,
    EVENT_BUTTON_TARA,
    EVENT_BUTTON_ATRAS,
} button_event_t;

extern QueueHandle_t central_queue;

extern SemaphoreHandle_t task_test_semaphore; // Semáforo binario para tarea test
extern SemaphoreHandle_t peso_semaphore; // Semáforo binario para el peso
extern SemaphoreHandle_t hall_semaphore; // Semáforo binario para el sensor de efecto hall
extern SemaphoreHandle_t ir_semaphore; // Semáforo binario para el sensor ir
extern SemaphoreHandle_t altura_semaphore; // Semáforo binario para el sensor ir
extern SemaphoreHandle_t button_semaphore; // Semáforo binario para el sensor ir
typedef enum {
    SENSOR_ALTURA=0,
    SENSOR_HALL,
    SENSOR_IR,
    SENSOR_BALANZA,
    TEST_TASK,
    BUTTON_EVENT
} sensor_origen_t;

typedef struct {
    sensor_origen_t origen;
    long altura;
    float peso;
    bool on_off;
    button_event_t button_event;
} data_t;

static const char *TAG;

#endif // __PROGRAM__