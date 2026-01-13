#include "inc/central.h"


float calcular_peso(long cuentas_raw_b1, long cuentas_raw_b2) {
    
    // 1. Calcular cuentas netas (Restar la TARA)
    long neto_b1 = cuentas_raw_b1 - TARA_BALANZA_1;
    long neto_b2 = cuentas_raw_b2 - TARA_BALANZA_2;

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



void nuevo_central(void *pvParameters) {

    static bool flag_tests = true;
    static bool flag_balanza1 = false;
    static bool flag_balanza2 = false;
    static bool flag_peso_calculado = false;

    central_data_t received_data;
    estados_central_t estado_actual = STATE_BIENVENIDA;
    
    static display_t display_data;
    
    uint32_t current_request_id = 0; // Contador incremental
    uint8_t expected_responses = 0;

    static long cuentas_raw_b1 = 0;
    static long cuentas_raw_b2 = 0;

    while (1) {
        // --- LÓGICA DE ENTRADA AL ESTADO --- //
        
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
                    printf("Error enviando pantalla TESTS.\n");
                }   

                break;

            case STATE_INICIAL:
                /* Se piden datos a todos los sensores:
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

                default:
                break;
        }

        // --- LÓGICA DE ESPERA Y PROCESAMIENTO --- //
        
        // Esperamos datos en cola.
        // Implementamos un timeout para no bloquearnos por siempre.
        uint8_t received_count = 0;

        while (received_count < expected_responses) {
            if (xQueueReceive(central_queue, &received_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
                
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


                // === CALCULO DE PESO (para los estados que lo necesiten) === //

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

                // --- ENVÍO DE DATOS Y LÓGICA DE TRANSICIÓN --- //
                // Los break son para salir del while de recepción y cambiar al siguiente estado.          
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

                }
                
                if (estado_actual == STATE_INICIAL) {
                    // Evaluo el teclado para cambiar de estado
                    if (received_data.origen == BUTTON_EVENT) {
                        if (received_data.button_event == EVENT_BUTTON_2){
                            estado_actual = STATE_BALANZA_RESUMEN;
                            break; 

                        } else if (received_data.button_event == EVENT_BUTTON_1){
                            estado_actual = STATE_APAGADO;
                            break; 
                        }
                    // Muestro por pantalla los valores recibidos de Altura, Freno y Barandas
                    } else if (received_data.origen == SENSOR_ALTURA){
                        display_data.data.altura = received_data.altura;
                    } else if (received_data.origen == SENSOR_FRENO){
                        display_data.data.freno_on_off = received_data.freno_on_off;
                    } else if (received_data.origen == SENSOR_HALL){
                        display_data.data.hall_on_off = received_data.hall_on_off;
                        if (display_data.data.hall_on_off == 0){
                            // Si no se detecta baranda cambio al estado Alerta Barandales
                            estado_actual = STATE_ALERTA_BARANDALES;
                            break;
                        }
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

        vTaskDelay(pdMS_TO_TICKS(200)); 


    } // Fin del while(1)

}


