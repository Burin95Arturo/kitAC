// Author: Burin Arturo
// Date: 01/08/2025

#ifndef __PROGRAM__
#define __PROGRAM__


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EVENT_BUTTON_UP,
    EVENT_BUTTON_DOWN,
    EVENT_BUTTON_SELECT,
} button_event_t;

extern QueueHandle_t button_event_queue;
extern QueueHandle_t weight_queue;       // Cola para los datos de peso (float)
extern QueueHandle_t height_queue;       // Cola para los datos de altura (float)
extern QueueHandle_t error_queue;       // Cola para los codigos de error (char)

#endif
