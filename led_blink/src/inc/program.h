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
extern QueueHandle_t display_queue;

extern SemaphoreHandle_t task_test_semaphore; // Semáforo binario para tarea test
extern SemaphoreHandle_t peso_semaphore; // Semáforo binario para el peso
extern SemaphoreHandle_t hall_semaphore; // Semáforo binario para el sensor de efecto hall
extern SemaphoreHandle_t ir_semaphore; // Semáforo binario para el sensor ir
extern SemaphoreHandle_t altura_semaphore; // Semáforo binario para el sensor de altura
extern SemaphoreHandle_t button_semaphore; // Semáforo binario para lo botones
extern SemaphoreHandle_t inclinacion_semaphore; // Semáforo binario para sensor de inclinación
extern SemaphoreHandle_t buzzer_semaphore; // Semáforo binario para el buzer
typedef enum {
    SENSOR_ALTURA=0,
    SENSOR_HALL,
    SENSOR_IR,
    SENSOR_BALANZA,
    SENSOR_GIROSCOPIO,
    TEST_TASK,
    BUTTON_EVENT
} sensor_origen_t;

typedef enum {
    NO_CHANGE=0,
    CHANGE, 
    WARNING
} states_display_t;
typedef struct {
    sensor_origen_t origen;
    long altura;
    int inclinacion;
    float peso;
    bool hall_on_off;
    bool ir_on_off;
    button_event_t button_event;
} data_t;

typedef struct {
    data_t data;
    states_display_t state;
} display_t;

// static const char *TAG;

#endif // __PROGRAM__