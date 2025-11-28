#include "inc/central.h"


void central_task(void *pvParameters) {

    static bool flag_balanza = false;
    static bool first_flag_balanza = false;
    static bool flag_cambiar_vista = false;
    data_t received_data;
    static data_t aux_data;
    static display_t display_data;
    static bool hall_change = false;
    static bool ir_change = false;
    static bool altura_change = false;
    static bool peso_change = false;
    static bool inclinacion_change = false;

    while (1) {

        // xSemaphoreGive(button_semaphore);
        xSemaphoreGive(inclinacion_semaphore);
        
        if (!flag_balanza) {
            // xSemaphoreGive(hall_semaphore);
            // xSemaphoreGive(ir_semaphore);  
            // xSemaphoreGive(altura_semaphore);
        }
        
        // Leer de la cola central_queue
        if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[Central] Origen: %d, Altura: %ld, Peso: %.2f, HALL_OnOff: %d, IR_OnOff: %d, Inclinacion: %f\n",
                received_data.origen,
                received_data.altura,
                received_data.peso,
                received_data.hall_on_off,
                received_data.ir_on_off,
                received_data.inclinacion);            
                        
            switch (received_data.origen)
            {
                case SENSOR_ALTURA:
                    altura_change = (received_data.altura != aux_data.altura) ? true : false;
                    aux_data.altura = received_data.altura;
                    break;
                
                case SENSOR_HALL:
                    hall_change = (received_data.hall_on_off != aux_data.hall_on_off) ? true : false;
                    aux_data.hall_on_off = received_data.hall_on_off;
                    break;
                
                case SENSOR_IR:
                    ir_change = (received_data.ir_on_off != aux_data.ir_on_off) ? true : false;
                    aux_data.ir_on_off = received_data.ir_on_off;           
                    break;
                
                case SENSOR_BALANZA:
                    // Procesar peso si es necesario
                    peso_change = (received_data.peso != aux_data.peso) ? true : false;
                    aux_data.peso = received_data.peso;
                    break;
                
                case SENSOR_ACELEROMETRO:
                    // Procesar inclinación si es necesario 
                    inclinacion_change = (received_data.inclinacion != aux_data.inclinacion) ? true : false;
                    aux_data.inclinacion = received_data.inclinacion;   
                    break;
                
                case BUTTON_EVENT:
                // Si es PESO o TARA, activa flag_balanza para dejar de tomar datos de los otros sensores
                // Si es ATRAS y flag_balanza estaba activo, implica que estaba en la función de peso. Entonces, desactiva flag_balanza
                // porque se entiende que quiere volver a la vista principal.
                // Si es ATRAS y flag_balanza inactivo, activa flag_cambiar_vista porque se entiende que se quiere cambiar el sensor que se muestra
                    if (received_data.button_event == EVENT_BUTTON_PESO || received_data.button_event == EVENT_BUTTON_TARA) {
                        flag_balanza = true;
//                        xSemaphoreGive(peso_semaphore);
                        first_flag_balanza = true;
                    }
                    else if (flag_balanza && received_data.button_event == EVENT_BUTTON_ATRAS) {
                        flag_balanza = false;
                        flag_cambiar_vista = false;
                    }
                    else {
                        flag_cambiar_vista = true;
                    }
                    aux_data.button_event = received_data.button_event;
                    break;
                
                default:
                    break;
            }       
            aux_data.origen = received_data.origen; 
            display_data.data = aux_data; 
        }

        // Prendo luz si la baranda esta baja
        if (!aux_data.hall_on_off) {
            gpio_set_level(LED_PIN, 1);
        } 

        // Nose si lo que viene va dentro del if anterior o no... yo supongo que no, pero es a corregir luego
        if (flag_balanza && first_flag_balanza) {
                first_flag_balanza = false;
            }
        else if (flag_balanza && !first_flag_balanza) {
            display_data.state = peso_change ? CHANGE : NO_CHANGE;
            peso_change = false;
            if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                printf("No se pudo enviar el peso a la cola.\n");
            }
        }
        else if (!flag_cambiar_vista && !flag_balanza) {
            display_data.state = inclinacion_change ? CHANGE : NO_CHANGE;
            inclinacion_change = false;
            display_data.data.origen = SENSOR_ACELEROMETRO;
            if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                printf("No se pudo enviar la inclinacion a la cola.\n");
            }
            printf("Inclinacion de %f enviada a la cola.\n", display_data.data.inclinacion);
            gpio_set_level(INTERNAL_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(INTERNAL_LED_PIN, 1); 
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(INTERNAL_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(INTERNAL_LED_PIN, 1); 
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(INTERNAL_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else {
            // Ver que se hace cuando se apreta el boton "atras" (o sea, el cambiar la vista)
            if (hall_change) {
                display_data.state = WARNING;
                hall_change = false;
                // xSemaphoreGive(buzzer_semaphore);
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola.\n");
                }
            }
            if (ir_change) {
                display_data.state = WARNING;
                ir_change = false;
                // xSemaphoreGive(buzzer_semaphore);
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola.\n");
                }
            }
            if (altura_change) {
                display_data.state = WARNING;
                altura_change = false;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola.\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}