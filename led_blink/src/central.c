#include "inc/central.h"

float calcular_peso(long cuentas_raw_b1, long cuentas_raw_b2) {
    
    // 1. Calcular cuentas netas (Restar la TARA)
    // long neto_b1 = cuentas_raw_b1 - TARA_BALANZA_1;
    // long neto_b2 = cuentas_raw_b2 - TARA_BALANZA_2;
    long neto_b1 = cuentas_raw_b1 - tara_b1;
    long neto_b2 = cuentas_raw_b2 - tara_b2;

    // 2. Calcular el peso parcial aportado por cada sector
    // Se usan coeficientes independientes porque la eficiencia mecánica es distinta
    float peso_parcial_1 = (float)neto_b1 / COEF_K1;
    float peso_parcial_2 = (float)neto_b2 / COEF_K2;

    // 3. Sumar para obtener el peso total
    float peso_total = peso_parcial_1 + peso_parcial_2;

    // 4. Filtrado básico de seguridad
    // Si el peso es negativo o muy pequeño (ruido), se devuelve 0
    if (peso_total < PESO_MINIMO_KG) {
        return 0.0f;
    }

    return peso_total;
}

void tara_balanzas(long raw_b1, long raw_b2) {
    tara_b1 = (int32_t)raw_b1;
    tara_b2 = (int32_t)raw_b2;

    // Persistimos los valores para que no se pierdan al reiniciar
    write_nvs_int("tara_b1", tara_b1);
    write_nvs_int("tara_b2", tara_b2);
    
    printf("Tara actualizada: B1=%ld, B2=%ld\n", tara_b1, tara_b2);
}


void nuevo_central(void *pvParameters) {

    static bool flag_tests = false;
    static bool flag_balanza1 = false;
    static bool flag_balanza2 = false;
    static bool flag_peso_calculado = false;
    static bool cabecera_en_horizontal = false;
    static bool freno_activado = false;
    static bool barandales_arriba = false;

    central_data_t received_data;
    estados_central_t estado_actual = STATE_BIENVENIDA;
    estados_central_t estado_anterior = STATE_BIENVENIDA;

    
    static display_t display_data;
    
    uint32_t current_request_id = 0; // Contador incremental
    uint8_t expected_responses = 0;

    static long cuentas_raw_b1 = 0;
    static long cuentas_raw_b2 = 0;

    static uint16_t contador_estado_pesando = 0;

    while (1) {

        // --------------------- BLOQUE 1: LÓGICA DE ENTRADA Y REPETICIÓN DEL ESTADO --------------------- //
        
        // Incrementamos el ID cada vez que pedimos nuevos datos.
        // Esto invalida automáticamente cualquier dato viejo en la cola.
        current_request_id++; 

        switch (estado_actual) {
            case STATE_BIENVENIDA:
                // New twist: Vamos a hacer que, si se toca algún botón en esta pantalla, se entra al modo test
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 1;

                display_data.contains_data = false; 
                display_data.pantalla = BIENVENIDA;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }

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
                    printf("Error enviando pantalla TESTS.\n");
                }   

                break;

            case STATE_INICIAL:
                /* Se piden datos a estos sensores:
                - Inclinación
                - Barandales 
                - Freno
                - Altura
                - Teclado
                */
                xTaskNotify(inclinacion_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(barandales_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(freno_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(altura_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 5;

                display_data.contains_data = false; 
                display_data.pantalla = INICIAL;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_ALERTA_BARANDALES:
                /* No se piden datos.
                */

                expected_responses = 0;

                display_data.contains_data = false; 
                display_data.pantalla = ALERTA_BARANDALES; //CAMBIAR PARA ALERTA_BARANDALES después del merge
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_APAGADO:
                /* Se piden datos a todos los sensores:
                - Teclado
                */
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 1;
                
                display_data.contains_data = false; 
                display_data.pantalla = APAGADA;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_ERROR_CABECERA:
                /* No se piden datos
                */
                expected_responses = 0;
                
                display_data.contains_data = false; 
                display_data.pantalla = ERROR_CABECERA;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_BALANZA_RESUMEN:
                /* Se piden datos a todos los sensores:     
                - Inclinación
                - Barandales 
                - Freno
                - Teclado
                */
                // Enviamos el current_request_id como valor de notificación
                xTaskNotify(barandales_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(freno_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(inclinacion_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 4;
                
                display_data.contains_data = false; 
                display_data.pantalla = BALANZA_RESUMEN;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_PESANDO:
                /* Se piden datos a todos los sensores:     
                - Balanza 1
                - Balanza 2
                */
                // Enviamos el current_request_id como valor de notificación
                xTaskNotify(balanza_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(balanza_2_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 2;
                
                display_data.contains_data = false; 
                display_data.pantalla = PESANDO;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_ERROR_FRENO:
                /* No se piden datos
                */
                expected_responses = 0;
                
                display_data.contains_data = false; 
                display_data.pantalla = ERROR_FRENO;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            case STATE_AJUSTE_CERO:
                /* Se piden datos a todos los sensores:     
                - Balanza 1
                - Balanza 2
                - Teclado
                - Inclinación
                */
                // Enviamos el current_request_id como valor de notificación

                // SEGUN "NUEVO ESQUEMA DE ESTADOS", BALANZA RESUMEN ES LA QUE CHEQUEA QUE ESTE TODO OK. AJUSTE CERO MIDE DIRECTO
                // xTaskNotify(inclinacion_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(teclado_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(balanza_task_handle, current_request_id, eSetValueWithOverwrite);
                xTaskNotify(balanza_2_task_handle, current_request_id, eSetValueWithOverwrite);

                expected_responses = 3;
                
                display_data.contains_data = false; 
                display_data.pantalla = AJUSTE_CERO;
                if (xQueueSend(display_queue, &display_data, (TickType_t)0) != pdPASS) {
                    printf("Error enviando pantalla INICIAL.\n");
                }
            break;

            default:
            break;
        } //Fin switch (estado_actual)

        // --------------------- FIN BLOQUE 1: LÓGICA DE ENTRADA Y REPETICIÓN DEL ESTADO --------------------- //

        // --------------------- BLOQUE 2: LÓGICA DE ESPERA Y PROCESAMIENTO ---------------------------------- //
        
        // Esperamos datos en cola.
        // Implementamos un timeout para no bloquearnos por siempre.
        uint8_t received_count = 0;

        while (received_count < expected_responses) {
            if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
                
                // ============================= FILTRO DE SEGURIDAD ======================================== //
                if (received_data.request_id != current_request_id) {
                    // Esto permite descartar datos viejos (solicitudes anteriores) que hayan quedado en la cola
                    ESP_LOGW("CENTRAL", "Descartando dato viejo del sensor %d (Batch %lu vs Actual %lu)", 
                             received_data.origen, received_data.request_id, current_request_id);
                    continue; // Ignoramos este mensaje y volvemos a leer la cola
                    // (este continue ignora lo que viene abajo y avanza con el while)
                }
                // Si llegamos aquí, el dato es FRESCO y válido para este estado
                received_count++;


                // =========== CALCULO DE PESO (para los estados que lo necesiten) ========================== //
                // Para las balanzas, se deben tomar datos de ambas para calcular el peso total
                if (received_data.origen == SENSOR_BALANZA){
                    cuentas_raw_b1 = received_data.peso_raw1;
                    flag_balanza1 = true;
                }         
                if (received_data.origen == SENSOR_BALANZA_2) {
                    cuentas_raw_b2 = received_data.peso_raw1;
                    flag_balanza2 = true;
                }
                if (flag_balanza1 && flag_balanza2) {
                    // Calcular peso total
                    received_data.peso_total = calcular_peso(cuentas_raw_b1, cuentas_raw_b2);

                    flag_balanza1 = false;
                    flag_balanza2 = false;
                    flag_peso_calculado = true;
                }
                // ============================= Fin CALCULO DE PESO ======================================== //

                // ====================== FLAG CABECERA EN HORIZONTAL ======================================= //
                if (received_data.origen == SENSOR_ACELEROMETRO) {

                    if (fabs(received_data.inclinacion) > ANGULO_MAXIMO_PERMITIDO) {
                        // Si el ángulo es mayor al permitido, cambiar al estado de error
                        cabecera_en_horizontal = false;
                    } else {
                        cabecera_en_horizontal = true;
                    }
                }
                // ====================== Fin FLAG CABECERA EN HORIZONTAL =================================== //
   
                // ====================== FLAG FRENO PUESTO ======================================= //
                if (received_data.origen == SENSOR_FRENO) {

                    if (received_data.freno_on_off) {
                        freno_activado = true;
                    } else {
                        freno_activado = false;
                    }
                }
                // ====================== Fin FLAG FRENO PUESTO =================================== //

                // ====================== FLAG BARANDALES ARRIBA ======================================= //
                if (received_data.origen == SENSOR_HALL) {

                    if (received_data.hall_on_off) {
                        barandales_arriba = true;
                    } else {
                        barandales_arriba = false;
                    }
                }
                // ====================== Fin FLAG BARANDALES ARRIBA =================================== //

                // ====================== APAGADO DE LED DE ALERTA ======================================= //
                if (freno_activado && barandales_arriba) {
                    gpio_set_level(LED_PIN, 0); // Apago LED de alerta
                }
                // ====================== Fin APAGADO DE LED DE ALERTA =================================== //

                // --------------------- BLOQUE 2.1: ENVÍO DE DATOS Y LÓGICA DE TRANSICIÓN ---------------------------------- //

                // Según el estado actual, se envían datos a la pantalla o se hacen transiciones.
                // Los break son para salir del while de recepción y cambiar al siguiente estado.
                
                /************************************** Estado TESTS *****************************************/
                if (estado_actual == STATE_TESTS) {
                    // Enviamos los datos recibidos a la cola del display
                    display_data.contains_data = true;
                    display_data.data.origen = received_data.origen;
                    // Si se lee teclaodo y no tiene datos, no se envía nada. (agregar eso)
                    display_data.data.inclinacion = received_data.inclinacion;

                    if (flag_peso_calculado) {
                        display_data.data.peso_total = received_data.peso_total;
                        display_data.data.origen = CALCULO_PESO;
                        flag_peso_calculado = false;
                    }

                    display_data.data.hall_on_off = received_data.hall_on_off;
                    display_data.data.freno_on_off = received_data.freno_on_off;
                    display_data.data.altura = received_data.altura;
                    display_data.data.button_event = received_data.button_event;

                    if (xQueueSend(display_queue, &display_data, 10 / portTICK_PERIOD_MS) != pdPASS) {
                        printf("Error enviando datos en pantalla TESTS.\n");

                    }
                    // TESTS no transiciona, queda siempre en este estado
                } // FIN ESTADO TESTS
                
                /************************************** Estado INICIAL *****************************************/
                if (estado_actual == STATE_INICIAL) {
                    // Evaluo el teclado para cambiar de estado
                    if (received_data.origen == BUTTON_EVENT) {
                        if (received_data.button_event == EVENT_BUTTON_1){
                            estado_actual = STATE_BALANZA_RESUMEN;
                            break;

                        } else if (received_data.button_event == EVENT_BUTTON_2){
                            estado_actual = STATE_APAGADO;
                            break;

                        } 
                    }
                    // Evaluo barandales para cambiar de estado
                    if (received_data.origen == SENSOR_HALL){
                        if (display_data.data.hall_on_off == 0){
                            // Si no se detecta baranda, cambio al estado Alerta Barandales
                            estado_actual = STATE_ALERTA_BARANDALES;
                            estado_anterior = STATE_INICIAL;
                            break;
                        }
                    }
                    // Muestro por pantalla los valores recibidos de Inclinación, Freno y Altura
                    if (received_data.origen == SENSOR_ACELEROMETRO  || received_data.origen == SENSOR_FRENO || received_data.origen == SENSOR_ALTURA) {
                        display_data.contains_data = true;
                        display_data.pantalla = INICIAL;
                        display_data.data.origen = received_data.origen;   
                        display_data.data.inclinacion = received_data.inclinacion;
                        display_data.data.freno_on_off = received_data.freno_on_off;
                        display_data.data.altura = received_data.altura;
                        if (xQueueSend(display_queue, &display_data, 10 / portTICK_PERIOD_MS) != pdPASS) {
                                printf("Error enviando datos en pantalla INICIAL.\n");
                        }
                        // Datos enviados correctamente
                        //else {
                        //    printf("Datos: Inclinacion: %.2f, Freno: %d, Altura: %ld, Origen: %d\n",
                        //           display_data.data.inclinacion,
                        //           display_data.data.freno_on_off,
                        //           display_data.data.altura,
                        //           display_data.data.origen);

                        //    }
                        

                    }

                } //FIN ESTADO INICIAL
            

                 /************************************** Estado APAGADO ********************************************/
                if (estado_actual == STATE_APAGADO) {
                    // Apago la pantalla

                    // Evaluo los botones
                    if (received_data.origen == BUTTON_EVENT) {
                        // Vuelvo al estado inicial
                        if (received_data.button_event != EVENT_NO_KEY){
                            estado_actual = STATE_INICIAL;
                            break;
                        }
                    } 
                }

                /************************************** Estado AJUSTE_CERO ****************************************/
                if (estado_actual == STATE_AJUSTE_CERO) {
                    if (received_data.origen == BUTTON_EVENT) {
                        if (received_data.button_event == EVENT_BUTTON_1){
                            // Voy para atras 
                            estado_actual = STATE_BALANZA_RESUMEN;
                            break;
                        }
                    } 
                    
                    else {
                        // Hay que ver si se envia algo al display o no
                        if (flag_peso_calculado){
                            tara_balanzas(received_data.peso_raw1, received_data.peso_raw2);
                        }
                    }
                }

                /************************************** Estado BALANZA_RESUMEN ***********************************/
                if (estado_actual == STATE_BALANZA_RESUMEN) {
                    // Evaluo los botones
                    if (received_data.origen == BUTTON_EVENT) {
                        if (received_data.button_event == EVENT_BUTTON_1){
                            // Voy al PESANDO si toqué 1 la cabecera está en horizontal. Si no está en horizontal, sale error

                            if (cabecera_en_horizontal && freno_activado && barandales_arriba){
                                estado_actual = STATE_PESANDO;
                                break;
                            } else if (!cabecera_en_horizontal){ 
                                estado_actual = STATE_ERROR_CABECERA;
                                estado_anterior = STATE_BALANZA_RESUMEN;
                                break;
                            }
                            else if (!freno_activado){
                                estado_actual = STATE_ERROR_FRENO;
                                estado_anterior = STATE_BALANZA_RESUMEN;
                                break;
                            }

                        } else if (received_data.button_event == EVENT_BUTTON_2){
                            // Guardo el dato en memoria
                            break; 
                        }
                        else if (received_data.button_event == EVENT_BUTTON_3){
                            if (cabecera_en_horizontal && freno_activado && barandales_arriba){
                                // Voy a ajustar el cero
                                estado_actual = STATE_AJUSTE_CERO;
                                break; 
                            } else if (!cabecera_en_horizontal){ 
                                estado_actual = STATE_ERROR_CABECERA;
                                estado_anterior = STATE_BALANZA_RESUMEN;
                                break;
                            }
                            else if (!freno_activado){
                                estado_actual = STATE_ERROR_FRENO;
                                estado_anterior = STATE_BALANZA_RESUMEN;
                                break;
                            }
                        }
                        else if (received_data.button_event == EVENT_BUTTON_4){
                            // Retorno al estado anterior
                            estado_actual = STATE_INICIAL;
                            break; 
                        }
                    }
                    if (barandales_arriba){
                        // Si no se detecta baranda cambio al estado Alerta Barandales
                        estado_actual = STATE_ALERTA_BARANDALES;
                        estado_anterior = STATE_BALANZA_RESUMEN;
                        break;
                    }

                    if (received_data.origen == SENSOR_FRENO) {
                        display_data.data.freno_on_off = received_data.freno_on_off;
                        display_data.contains_data = true;
                        display_data.pantalla = BALANZA_RESUMEN;
                        if (xQueueSend(display_queue, &display_data, 10 / portTICK_PERIOD_MS) != pdPASS) {
                                printf("Error enviando datos en pantalla BALANZA_RESUMEN.\n");
                        }
                    }
                } //FIN ESTADO BALANZA_RESUMEN

                /************************************** Estado PESANDO ***************************************/
                if (estado_actual == STATE_PESANDO) {

                    if (flag_peso_calculado) {
                        flag_peso_calculado = false;
                        contador_estado_pesando++;
                    }
                    
                    // if (received_data.origen == CALCULO_PESO) {
                    //     // Envio el valor del peso a display.
                    //     display_data.data.peso_total = received_data.peso_total;
                    //     display_data.contains_data = true;
                    //     display_data.pantalla = PESANDO;
                    //     display_data.data.origen = CALCULO_PESO;
                    //     if (xQueueSend(display_queue, &display_data, 10 / portTICK_PERIOD_MS) != pdPASS) {
                    //         printf("Error enviando datos en pantalla PESANDO.\n");
                    //     }
                    // }
                    
                    if (contador_estado_pesando >= MUESTRAS_PROMEDIO) {
                        // Después de obtener suficientes muestras, vuelvo a BALANZA_RESUMEN
                        // Envio el valor del peso a display.

                        display_data.data.peso_total = received_data.peso_total;
                        display_data.contains_data = true;
                        display_data.pantalla = PESANDO;
                        display_data.data.origen = CALCULO_PESO;
                        if (xQueueSend(display_queue, &display_data, 10 / portTICK_PERIOD_MS) != pdPASS) {
                            printf("Error enviando datos en pantalla PESANDO.\n");
                        }

                        contador_estado_pesando = 0;
                        estado_actual = STATE_BALANZA_RESUMEN;
                        break;
                    }

                } // Fin ESTADO PESANDO
                
                if (estado_actual == STATE_BIENVENIDA) {
                    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 seg
                    
                    // Después de 3 segundos, evaluar si se presionó algún botón (por ahora, cualquiera)
                    // Si hay algún botón en cola, significa que se quiere entrar a TESTS
                    if (received_data.origen == BUTTON_EVENT && received_data.button_event != EVENT_NO_KEY) {
                        estado_actual = STATE_TESTS;
                        break;
                    } else {
                        estado_actual = STATE_INICIAL;
                        break;
                    } 
                } // Fin ESTADO BIENVENIDA
                
            } else {
                // Si no se leyó datos en la cola y se cumplió el timeout esperando sensores
                break; 
            }
            // ------------ FIN BLOQUE 2.1: ENVÍO DE DATOS Y LÓGICA DE TRANSICIÓN -------------- //

        } // Fin del while de recepción
        

        // --------------------- BLOQUE 3: ESTADOS QUE NO DEPENDEN DE DATOS EN LA COLA ---------------------------------- //

        // Transicionan solos después de un tiempo 
        if (estado_actual == STATE_ALERTA_BARANDALES) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 seg

            estado_actual = estado_anterior;

            xSemaphoreGive(buzzer_semaphore);
        }

        // Transicionan después de un tiempo mostrando el Error Cabecera
        // flag_test = false funcionamiento normal fuera del modo test STATE_TEST
        // Uso dicho flag para saber de donde vino y a donde retorno desde Error Cabecera
        // flag_test = false -> vino desde STATE_BALANZA_RESUMEN
        // flag_test = true -> vino desde STATE_AJUSTE_CERO 

        if (estado_actual == STATE_ERROR_CABECERA) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 seg
            
            estado_actual = estado_anterior;

//            xSemaphoreGive(buzzer_semaphore);
            // ACA SE TIENE QUE PRENDER UN LED ROJO Y EL BUZZER --> a chequear

        }

        if (estado_actual == STATE_ERROR_FRENO) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar 3 seg
            
            estado_actual = estado_anterior;

            xSemaphoreGive(buzzer_semaphore);

            // ACA SE TIENE QUE PRENDER UN LED ROJO Y EL BUZZER

        }

        // --------------------- FIN BLOQUE 3: ESTADOS QUE NO DEPENDEN DE DATOS EN LA COLA ---------------------------------- //


        /*********** Demora general para el bucle completo ********************/
        vTaskDelay(pdMS_TO_TICKS(100)); 

    } // Fin del while(1)

}


