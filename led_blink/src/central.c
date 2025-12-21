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
    static bool break_change = false;
    static bool altura_change = false;
    static bool peso_change_1 = false;
    static bool peso_change_2 = false;    
    static bool inclinacion_change = false;
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Esperar 2 segundos para que se inicialicen otros componentes
    while (1) {

        xSemaphoreGive(button_semaphore);
        //xSemaphoreGive(break_semaphore);
        //xSemaphoreGive(inclinacion_semaphore);

        
        if (!flag_balanza) {
            xSemaphoreGive(ir_semaphore);  
            xSemaphoreGive(inclinacion_semaphore);
            xSemaphoreGive(hall_semaphore);
            xSemaphoreGive(altura_semaphore);
        }
        
        // Leer de la cola central_queue
        if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
            // printf("[Central] Origen: %d, Altura: %ld, Peso_1: %.2f, Peso_2: %.2f, HALL_OnOff: %d, IR_OnOff: %d, Inclinacion: %f, Freno: %d\n",
            //     received_data.origen,
            //     received_data.altura,
            //     received_data.peso_1,
            //     received_data.peso_2,
            //     received_data.hall_on_off,
            //     received_data.ir_on_off,
            //     received_data.inclinacion,
            //     received_data.freno_on_off);   
            
            received_data.altura = 100;
            received_data.peso_total = 50.5f;
            received_data.hall_on_off = 0;
            received_data.ir_on_off = 1;
            received_data.freno_on_off = 0;   
                    
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
                
                case SENSOR_FRENO:
                    break_change = (received_data.freno_on_off != aux_data.freno_on_off) ? true : false;
                    aux_data.freno_on_off = received_data.freno_on_off;    
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
                //    inclinacion_change = (received_data.inclinacion != aux_data.inclinacion) ? true : false;
                    inclinacion_change = true;
                    aux_data.inclinacion = received_data.inclinacion;   
                    printf("Inclinacion recibida: %f\n", received_data.inclinacion);
                    break;
                
                case BUTTON_EVENT:
                // Si es PESO o TARA, activa flag_balanza para dejar de tomar datos de los otros sensores
                // Si es ATRAS y flag_balanza estaba activo, implica que estaba en la función de peso. Entonces, desactiva flag_balanza
                // porque se entiende que quiere volver a la vista principal.
                // Si es ATRAS y flag_balanza inactivo, activa flag_cambiar_vista porque se entiende que se quiere cambiar el sensor que se muestra
                    // if (received_data.button_event == EVENT_BUTTON_PESO || received_data.button_event == EVENT_BUTTON_TARA) {
                    //     flag_balanza = true;
                    //     xSemaphoreGive(peso_semaphore);
                    //     xSemaphoreGive(peso_semaphore_2);
                    //     first_flag_balanza = true;
                    // }
                    // else if (flag_balanza && received_data.button_event == EVENT_BUTTON_ATRAS) {
                    //     flag_balanza = false;
                    //     flag_cambiar_vista = false;
                    //     xSemaphoreTake(peso_semaphore, portMAX_DELAY);
                    //     xSemaphoreTake(peso_semaphore_2, portMAX_DELAY);
                    // }
                    // else {
                    //     flag_cambiar_vista = true;
                    // }

                        display_data.data.origen = BUTTON_EVENT;
                        display_data.pantalla = TESTS;
                        if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                            printf("No se pudo enviar informacion a la cola display.\n");
                        }
                        
                        printf("[Central_2] Origen: %d, Altura: %ld, Peso_total: %.2f, HALL_OnOff: %d, IR_OnOff: %d, Inclinacion: %f, Freno: %d, Tecla: %d\n",
                        display_data.data.origen,
                        display_data.data.altura,
                        display_data.data.peso_total,
                        display_data.data.hall_on_off,
                        display_data.data.inclinacion,
                        display_data.data.freno_on_off,
                        display_data.data.button_event); 
   

                    break;
                
                default:
                    break;
            } 
            aux_data.origen = received_data.origen;       
            //display_data.data = aux_data;
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
        // else if (!flag_cambiar_vista && !flag_balanza) {
        //     display_data.state = inclinacion_change ? CHANGE : NO_CHANGE;
        //     inclinacion_change = false;
        //     display_data.data.origen = SENSOR_ACELEROMETRO;
        //     display_data.pantalla = TESTS;
        //     if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
        //         printf("No se pudo enviar la inclinacion a la cola display.\n");
        //     }
        //}
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
            if (break_change) {
                display_data.state = WARNING;
                break_change = false;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }
            }
            if (inclinacion_change) {
                display_data.state = WARNING;
                display_data.pantalla = TESTS;
                //inclinacion_change = false;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }
                
                printf("[Central_3] Origen: %d, Altura: %ld, Peso_total: %.2f, HALL_OnOff: %d, IR_OnOff: %d, Inclinacion: %f, Freno: %d\n",
                display_data.data.origen,
                display_data.data.altura,
                display_data.data.peso_total,
                display_data.data.hall_on_off,

                display_data.data.inclinacion,
               display_data.data.freno_on_off);   

            vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



void nuevo_central(void *pvParameters) {

    static bool flag_tests = true;

    central_data_t received_data;
    estados_central_t estado_actual = STATE_BIENVENIDA;
    
    static display_t display_data;
    
    uint32_t current_request_id = 0; // Contador incremental
    uint8_t expected_responses = 0;

    while (1) {
        // --- LÓGICA DE ENTRADA AL ESTADO (TRIGGER) --- //
        
        // Incrementamos el ID cada vez que pedimos nuevos datos.
        // Esto invalida automáticamente cualquier dato viejo en la cola.
        current_request_id++; 

        switch (estado_actual) {
            case STATE_BIENVENIDA:
                // Lo único que se hacemos al entrar es esperar un tiempo y luego cambiar a INICIAL.
                // o a TESTS si el flag está activo.
                expected_responses = 0;

                break;

            case STATE_TESTS:
                /* Se piden datos a todos los sensores:
                - Inclinación
                - Balanza 1
                - Balanza 2
                - Barandales 
                - Freno
                - Altura
                - Teclado
                */
                // Enviamos el current_request_id como valor de notificación
                xTaskNotify(inclinacion_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(balanza_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(balanza_2_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(barandales_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(freno_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(altura_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 7;

                // Cuando se ingresa al estado por primera vez, esto es para solo 
                // dibujar la pantalla de tests sin datos
                display_data.contains_data = false; 
                display_data.pantalla = TESTS;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("No se pudo enviar informacion a la cola display.\n");
                }   

                break;

            case STATE_INICIAL:
                // En Estado A, pedimos datos a sensores 1, 2 y 3
                // Enviamos el current_request_id como valor de notificación
                // En Estado B, quizás solo pedimos al sensor 4 y 5
                xTaskNotify(balanza_task_handle, current_request_id, eSetValueWithOverwrite);
                break;

                default:
                break;
        }

        // --- LÓGICA DE ESPERA Y PROCESAMIENTO --- //
        
        // Esperamos datos en cola.
        // Implementamos un timeout para no bloquearnos por siempre.
        uint8_t received_count = 0;

        while (received_count < expected_responses) {
            if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(500)) == pdTRUE) {
                
                // === FILTRO DE SEGURIDAD === //
                if (received_data.request_id != current_request_id) {
                    // Esto permite descartar datos viejos (solicitudes anteriores) que hayan quedado en la cola
                    ESP_LOGW("CENTRAL", "Descartando dato viejo del sensor %d (Batch %lu vs Actual %lu)", 
                             received_data.origen, received_data.request_id, current_request_id);
                    continue; // Ignoramos este mensaje y volvemos a leer la cola
                    // (este continue ignora lo que viene abajo y avanza con el while)
                }
                // Si llegamos aquí, el dato es FRESCO y válido para este estado
                received_count++;


                // --- ENVÍO DE DATOS Y LÓGICA DE TRANSICIÓN --- //
                // Los break son para salir del while de recepción y cambiar al siguiente estado.          
                if (estado_actual == STATE_TESTS) {
                    // Enviamos los datos recibidos a la cola del display
                    display_data.contains_data = true;
                    display_data.data.origen = received_data.origen;
                    display_data.data.inclinacion = received_data.inclinacion;
                    display_data.data.peso_total = received_data.peso_total;
                    display_data.data.hall_on_off = received_data.hall_on_off;
                    display_data.data.freno_on_off = received_data.freno_on_off;
                    display_data.data.altura = received_data.altura;
                    display_data.data.button_event = received_data.button_event;

                    if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                        printf("No se pudo enviar informacion a la cola display.\n");
                    }   

                    // TESTS no transiciona, queda siempre en este estado

                }
                
                if (estado_actual == STATE_INICIAL) {
                     if (received_data.peso_total > 50.0) {
                         // El sensor 2 dispara el cambio.
                         // Nota: Aún faltaba leer el sensor 3, pero al hacer break aquí,
                         // salimos del while, el loop principal da la vuelta,
                         // current_request_id se incrementa, y cuando el sensor 3 
                         // finalmente escriba en la cola (o si ya escribió), 
                         // su mensaje tendrá el ID viejo y será descartado en el futuro.
                         
                         estado_actual = STATE_PESANDO;
                         break; // Salimos del loop de recepción para cambiar de estado
                     }
                }
                
            } else {
                // Timeout esperando sensores
                break; 
            }
        } // Fin del while de recepción
        
        
        // --- ESTADOS QUE NO DEPENDEN DE DATOS EN LA COLA --- //
        // Transicionan solos después de un tiempo 
        if (estado_actual == STATE_BIENVENIDA) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 seg
            
            if (flag_tests) {
                estado_actual = STATE_TESTS;
                flag_tests = false;
            } else {                        
            estado_actual = STATE_INICIAL; 
            }
        }


    } // Fin del while(1)

}


