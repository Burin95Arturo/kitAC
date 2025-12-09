#include "inc/central.h"




void calcular_peso(data_t *peso_data) {
    static float vector_peso_1 [MUESTRAS_PROMEDIO] = {0};
    static float vector_peso_2 [MUESTRAS_PROMEDIO] = {0};
    static int index_1 = 0;
    static int index_2 = 0;
    float prom_1 = 0.0f;
    float prom_2 = 0.0f;
    bool finish_prom_1 = false;
    bool finish_prom_2 = false;

    switch (peso_data->origen) {
        case SENSOR_BALANZA:
            if (index_1 < MUESTRAS_PROMEDIO) {
                vector_peso_1[index_1] = peso_data->peso_1;
                index_1++;
            }
            else {
                // Calcular promedio
                float suma_1 = 0.0f;
                for (int i = 0; i < MUESTRAS_PROMEDIO; i++) {
                    suma_1 += vector_peso_1[i];
                }
                prom_1 = suma_1 / MUESTRAS_PROMEDIO;
                finish_prom_1 = true;
            }
            break;
        
        case SENSOR_BALANZA_2:
            if (index_2 < MUESTRAS_PROMEDIO) {
                vector_peso_2[index_2] = peso_data->peso_2;
                index_2++;
            }
            else {
                // Calcular promedio
                float suma_2 = 0.0f;
                for (int j = 0; j < MUESTRAS_PROMEDIO; j++) {
                    suma_2 += vector_peso_2[j];
                }
                prom_2 = suma_2 / MUESTRAS_PROMEDIO;
                finish_prom_2 = true;
            }
            break;

        default:
            break;
    }

    if (finish_prom_1 && finish_prom_2) {
        peso_data->peso_total = (prom_1 + prom_2)/2.0f;
        finish_prom_1 = false;
        finish_prom_2 = false;
        index_1 = 0; // Reiniciar índice para la próxima ronda de muestras
        index_2 = 0; // Reiniciar índice para la próxima ronda de muestras        
    }

    printf("Calculando peso...\n");
}

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
    static bool peso_change_1 = false;
    static bool peso_change_2 = false;    
    static bool inclinacion_change = false;

    while (1) {

        xSemaphoreGive(button_semaphore);
        
        
        if (!flag_balanza) {
            xSemaphoreGive(ir_semaphore);  
            xSemaphoreGive(inclinacion_semaphore);
            xSemaphoreGive(peso_semaphore);
            xSemaphoreGive(peso_semaphore_2);
            xSemaphoreGive(hall_semaphore);
            xSemaphoreGive(altura_semaphore);
        }
        
        // Leer de la cola central_queue
        if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[Central] Origen: %d, Altura: %ld, Peso_1: %.2f, Peso_2: %.2f, HALL_OnOff: %d, IR_OnOff: %d, Inclinacion: %f\n",
                received_data.origen,
                received_data.altura,
                received_data.peso_1,
                received_data.peso_2,
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
                    peso_change_1 = (received_data.peso_1 != aux_data.peso_1) ? true : false;
                    aux_data.peso_1 = received_data.peso_1;
                    xSemaphoreGive(peso_semaphore);
                    break;
                
                case SENSOR_BALANZA_2:
                    peso_change_2 = (received_data.peso_2 != aux_data.peso_2) ? true : false;
                    aux_data.peso_2 = received_data.peso_2;    
                    xSemaphoreGive(peso_semaphore_2);
                    break;
                
                case SENSOR_ACELEROMETRO:
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
                        xSemaphoreGive(peso_semaphore);
                        xSemaphoreGive(peso_semaphore_2);
                        first_flag_balanza = true;
                    }
                    else if (flag_balanza && received_data.button_event == EVENT_BUTTON_ATRAS) {
                        flag_balanza = false;
                        flag_cambiar_vista = false;
                        xSemaphoreTake(peso_semaphore, portMAX_DELAY);
                        xSemaphoreTake(peso_semaphore_2, portMAX_DELAY);
                    }
                    else {
                        flag_cambiar_vista = true;
                    }

                    break;
                
                default:
                    break;
            } 
            aux_data.origen = received_data.origen;       
            display_data.data = aux_data;
        }

        // Prendo luz si la baranda esta baja
        if (!aux_data.hall_on_off) {
        //    gpio_set_level(LED_PIN, 1);
        } 

        if (flag_balanza && first_flag_balanza) {
                first_flag_balanza = false;
            }
        else if (flag_balanza && !first_flag_balanza) {
            display_data.state = (peso_change_1 ? CHANGE : NO_CHANGE) || (peso_change_2 ? CHANGE : NO_CHANGE);
            peso_change_1 = false;
            peso_change_2 = false;
            calcular_peso(&display_data.data);
            if (display_data.data.origen == CALCULO_PESO) {
                // Enviar el peso calculado a la cola
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar el peso a la cola display.\n");
                }
                printf("Peso total calculado: %.2f kg\n", display_data.data.peso_total);
            }
        }
        else if (!flag_cambiar_vista && !flag_balanza) {
            display_data.state = inclinacion_change ? CHANGE : NO_CHANGE;
            inclinacion_change = false;
            display_data.data.origen = SENSOR_ACELEROMETRO;
            if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                printf("No se pudo enviar la inclinacion a la cola display.\n");
            }
        }
        else {
            // Ver que se hace cuando se apreta el boton "atras" (o sea, el cambiar la vista)
            if (hall_change) {
                display_data.state = WARNING;
                hall_change = false;
                xSemaphoreGive(buzzer_semaphore);
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }
            }
            if (ir_change) {
                display_data.state = WARNING;
                ir_change = false;
                xSemaphoreGive(buzzer_semaphore);
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }
            }
            if (altura_change) {
                display_data.state = WARNING;
                altura_change = false;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
