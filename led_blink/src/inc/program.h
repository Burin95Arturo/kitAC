#ifndef __PROGRAM__
#define __PROGRAM__


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "freertos/semphr.h"

// Número de lecturas por solicitud (ajustable)
#ifndef HX711_READ_ITERATIONS
#define HX711_READ_ITERATIONS 10
#endif

// --- PARÁMETROS DE CALIBRACIÓN DE LAS BALANZAS ---
// Calculados basándose en la tabla de datos y TARA
#define TARA_BALANZA_1  192044  // Valor en vacío HX711 n.º 1
#define TARA_BALANZA_2  509788  // Valor en vacío HX711 n.º 2

// Factores de división (Cuentas por Kg)
// K1 es menor porque las "patitas" absorben peso, generando menos cuentas por Kg real.
#define COEF_K1         23870.0f 
#define COEF_K2         27620.0f

// Umbral mínimo para mostrar peso (filtro de ruido)
#define PESO_MINIMO_KG  2.0f

// Umbral mínimo para indicar posición segura (altura en unidades del sensor)
#define ALTURA_SEGURA_MIN_CM  40  // Ajustable según cama

// Nivel para detección de freno activado (botón presionado)
#define NIVEL_FRENO_ACTIVADO  0  // Ajustable según conexión física

typedef enum {
    EVENT_BUTTON_1=1,
    EVENT_BUTTON_2,
    EVENT_BUTTON_3,
    EVENT_BUTTON_4,
    EVENT_NO_KEY,
} button_event_t;

extern QueueHandle_t central_queue;
extern QueueHandle_t display_queue;

extern SemaphoreHandle_t task_test_semaphore; // Semáforo binario para tarea test
extern SemaphoreHandle_t peso_semaphore; // Semáforo binario para el peso
extern SemaphoreHandle_t peso_semaphore_2; // Semáforo binario para el peso
extern SemaphoreHandle_t hall_semaphore; // Semáforo binario para el sensor de efecto hall
extern SemaphoreHandle_t ir_semaphore; // Semáforo binario para el sensor ir
extern SemaphoreHandle_t altura_semaphore; // Semáforo binario para el sensor de altura
extern SemaphoreHandle_t button_semaphore; // Semáforo binario para lo botones
extern SemaphoreHandle_t inclinacion_semaphore; // Semáforo binario para sensor de inclinación
extern SemaphoreHandle_t buzzer_semaphore; // Semáforo binario para el buzer
extern SemaphoreHandle_t break_semaphore; // Semáforo binario para la función de break

extern TaskHandle_t inclinacion_task_handle;
extern TaskHandle_t balanza_task_handle;
extern TaskHandle_t balanza_2_task_handle;
extern TaskHandle_t barandales_task_handle;
extern TaskHandle_t altura_task_handle;
extern TaskHandle_t freno_task_handle;
extern TaskHandle_t teclado_task_handle;
typedef enum {
    SENSOR_ALTURA=0,
    SENSOR_HALL,
    SENSOR_IR,
    SENSOR_BALANZA,
    SENSOR_BALANZA_2,
    SENSOR_FRENO,
    CALCULO_PESO,
    SENSOR_ACELEROMETRO,
    TEST_TASK,
    BUTTON_EVENT
} sensor_origen_t;
typedef enum {
    NO_CHANGE=0,
    CHANGE, 
    WARNING
} states_display_t; //Borrar una vez terminada la nueva estructura de central 
typedef struct {
    sensor_origen_t origen;
    long altura;
    float inclinacion;
    float peso_1;
    float peso_2;
    float peso_total;
    bool hall_on_off;
    bool ir_on_off;
    bool freno_on_off;
    button_event_t button_event;
} data_t;  //Borrar una vez terminada la nueva estructura de central 

typedef struct {
    sensor_origen_t origen;
    long altura;
    float inclinacion;
    long peso_raw1;
    long peso_raw2;
    float peso_1;
    float peso_2;
    float peso_total; //Borrar pesos, quedarse con los raw
    bool hall_on_off;
    bool freno_on_off;
    button_event_t button_event;
    uint32_t request_id;
} central_data_t;


typedef enum {
    BIENVENIDA=0,
    APAGADA,
    INICIAL, 
    BALANZA_RESUMEN,
    TESTS,
    CONFIGURACION
} pantallas_t;

typedef struct {
    central_data_t data;
    bool contains_data;
    states_display_t state;
    pantallas_t pantalla;
} display_t;

typedef enum {
    STATE_BIENVENIDA,
    STATE_TESTS,
    STATE_INICIAL,
    STATE_APAGADO,
    STATE_ALERTA_BARANDALES,
    STATE_BALANZA_RESUMEN,
    STATE_PESANDO,
    STATE_ERROR_CABECERA,
    STATTE_AJUSTE_CERO,    
    // ... otros estados
} estados_central_t;


// static const char *TAG;

#endif // __PROGRAM__