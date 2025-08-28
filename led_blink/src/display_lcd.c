// Author: Burin Arturo
// Date: 20/06/2025

#include "inc/display_lcd.h"


// --- Definiciones para el Display ---
#define LCD_COLS 8 // Número de columnas (caracteres por línea)
#define LCD_ROWS 2 // Número de filas (líneas)

#define NUM_MENU_OPTIONS (sizeof(menu_options) / sizeof(menu_options[0]))

// --- Prototipos de funciones de acciones del menu ---
static void balance_tare_action(void);
static void balance_weigh_action(void);
static void balance_calibrate_action(void);
static void balance_errors_action(void);
static void inclination_head_action(void);
static void inclination_foot_action(void);
static void inclination_errors_action(void);
static void height_status_action(void);
static void height_errors_action(void);
static void railings_status_action(void);
static void railings_errors_action(void);
static void brake_status_action(void);
static void brake_errors_action(void);
static void idle_000_action(void);
static void idle_001_action(void);
static void idle_010_action(void);
static void idle_011_action(void);
static void idle_100_action(void);

// --- Estado del menu ---
typedef enum {
    MENU_STATE_NAVIGATING,  // Estado normal de navegacion del menu
    MENU_STATE_WEIGHING,    // Estado especial para mostrar el peso
    MENU_STATE_TARAR,       // Estado especial para mostrar el peso de la tara
    MENU_STATE_HEIGHT,      // Estado especial para mostrar la altura
    MENU_STATE_ERRORS       // Estado especial para mostrar errores
} menu_state_t;

// --- Estructura para un elemento de menu ---
// Un elemento puede tener un texto, una acción y un puntero a un submenú.
typedef struct menu_item_t {
    const char *text;
    void (*action)(void); // Puntero a la función que se ejecuta al seleccionar
    struct menu_item_t *submenu; // Puntero al submenú (si existe)
    int submenu_size; // Tamaño del submenú
} menu_item_t;

// --- Declaración de los menús (de abajo hacia arriba) ---
// Primero se definen los submenús y luego el menú principal.

// Menú: Inclinacion
static menu_item_t menu_options_inclinacion[] = {
    {"Cabecera", inclination_head_action, NULL, 0},
    {"Piecera", inclination_foot_action, NULL, 0},
    {"Errores", inclination_errors_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_INCLINACION (sizeof(menu_options_inclinacion) / sizeof(menu_item_t))

// Menú: Balanza
static menu_item_t menu_options_balanza[] = {
    {"Tarar", balance_tare_action, NULL, 0},
    {"Pesar", balance_weigh_action, NULL, 0},
    {"Calibrar", balance_calibrate_action, NULL, 0},
    {"Errores", balance_errors_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_BALANZA (sizeof(menu_options_balanza) / sizeof(menu_item_t))

// Menú: Altura
static menu_item_t menu_options_altura[] = {
    {"Altura", height_status_action, NULL, 0},
    {"Errores", height_errors_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_ALTURA (sizeof(menu_options_altura) / sizeof(menu_item_t))

// Menú: Barandas
static menu_item_t menu_options_barandas[] = {
    {"Barandas", railings_status_action, NULL, 0},
    {"Errores", railings_errors_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_BARANDAS (sizeof(menu_options_barandas) / sizeof(menu_item_t))

// Menú: Freno
static menu_item_t menu_options_freno[] = {
    {"Freno", brake_status_action, NULL, 0},
    {"Errores", brake_errors_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_FRENO (sizeof(menu_options_freno) / sizeof(menu_item_t))

// Menú: "..."
static menu_item_t menu_options_idle[] = {
    {"000", idle_000_action, NULL, 0},
    {"001", idle_001_action, NULL, 0},
    {"010", idle_010_action, NULL, 0},
    {"011", idle_011_action, NULL, 0},
    {"100", idle_100_action, NULL, 0},
    {"Atras", NULL, NULL, 0},
};
#define NUM_MENU_OPTIONS_IDLE (sizeof(menu_options_idle) / sizeof(menu_item_t))

// Menú principal
static menu_item_t main_menu[] = {
    {"Balanza", NULL, menu_options_balanza, NUM_MENU_OPTIONS_BALANZA},
    {"Inclinacion", NULL, menu_options_inclinacion, NUM_MENU_OPTIONS_INCLINACION},
    {"Altura", NULL, menu_options_altura, NUM_MENU_OPTIONS_ALTURA},
    {"Barandas", NULL, menu_options_barandas, NUM_MENU_OPTIONS_BARANDAS},
    {"Freno", NULL, menu_options_freno, NUM_MENU_OPTIONS_FRENO},
    {"...", NULL, menu_options_idle, NUM_MENU_OPTIONS_IDLE},
};
#define NUM_MAIN_MENU_OPTIONS (sizeof(main_menu) / sizeof(menu_item_t))

// --- TAG para los logs ---
static const char *TAG = "LCD_DISPLAY";
static const char *TAG_MENU = "MENU_TASK";

// --- Cadenas Globales para el Contenido del Display ---
// Estas cadenas serán actualizadas por otras tareas (ej. la tarea de la balanza)
// volatile para asegurar que el compilador no optimice lecturas/escrituras.
// Considera usar un mutex si múltiples tareas escriben en estas cadenas para evitar corrupciones.
volatile char g_lcd_line1[LCD_COLS + 1] = "        "; // Línea 1, inicializada con espacios
volatile char g_lcd_line2[LCD_COLS + 1] = "        "; // Línea 2, inicializada con espacios

// --- Prototipos de funciones internas del driver LCD ---
static void lcd_send_nibble(uint8_t nibble);
static void lcd_send_command(uint8_t command);
static void lcd_send_data(uint8_t data);
static void lcd_pulse_enable(void);

// Menú de opciones
const char *menu_options[] = {
    "Balanza",
    "Inclinacion",
    "Altura",
    "Barandas",
    "Freno",
    "..."
};

// Variable de estado del menú (global, pero accedida solo por la tarea del menú)
static int current_selection = 0;
static int prev_selection_index = 0;
static int num_options = 0;

// --- Variables de estado del menu ---
static menu_item_t *current_menu_ptr = main_menu;
static menu_item_t *parent_menu_ptr = NULL;
static menu_state_t current_state = MENU_STATE_NAVIGATING;
static float current_weight_display = 0.0f;
static float current_height_display = 0.0f;
static char current_error_display[8];

/**
 * @brief Envía un nibble (4 bits) al bus de datos del LCD.
 * @param nibble El nibble a enviar (bits 0-3).
 */
static void lcd_send_nibble(uint8_t nibble) {
    gpio_set_level(LCD_DB4_PIN, (nibble >> 0) & 0x01);
    gpio_set_level(LCD_DB5_PIN, (nibble >> 1) & 0x01);
    gpio_set_level(LCD_DB6_PIN, (nibble >> 2) & 0x01);
    gpio_set_level(LCD_DB7_PIN, (nibble >> 3) & 0x01);
}

/**
 * @brief Genera un pulso en el pin Enable (E) para que el LCD lea los datos.
 */
static void lcd_pulse_enable(void) {
    gpio_set_level(LCD_E_PIN, 1);
    esp_rom_delay_us(1); // Pequeño delay para asegurar el pulso
    gpio_set_level(LCD_E_PIN, 0);
    esp_rom_delay_us(50); // Delay después del pulso para que el LCD procese (min. 37us)
}

/**
 * @brief Envía un comando al LCD.
 * @param command El comando de 8 bits a enviar.
 */
static void lcd_send_command(uint8_t command) {
    gpio_set_level(LCD_RS_PIN, 0); // RS en LOW para modo comando
    // Enviar nibble alto
    lcd_send_nibble(command >> 4);
    lcd_pulse_enable();
    // Enviar nibble bajo
    lcd_send_nibble(command & 0x0F);
    lcd_pulse_enable();
}

/**
 * @brief Envía un dato (carácter) al LCD.
 * @param data El dato de 8 bits a enviar.
 */
static void lcd_send_data(uint8_t data) {
    gpio_set_level(LCD_RS_PIN, 1); // RS en HIGH para modo dato
    // Enviar nibble alto
    lcd_send_nibble(data >> 4);
    lcd_pulse_enable();
    // Enviar nibble bajo
    lcd_send_nibble(data & 0x0F);
    lcd_pulse_enable();
}


/**
 * @brief Inicializa el display LCD en modo de 4 bits.
 */
void lcd_init(void) {
    // Configurar pines GPIO para el LCD como salidas
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_RS_PIN) | (1ULL << LCD_E_PIN) |
                        (1ULL << LCD_DB4_PIN) | (1ULL << LCD_DB5_PIN) |
                        (1ULL << LCD_DB6_PIN) | (1ULL << LCD_DB7_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "LCD: Pines GPIO inicializados.");

    // Asegurarse de que E y RS estén en LOW al inicio
    gpio_set_level(LCD_E_PIN, 0);
    gpio_set_level(LCD_RS_PIN, 0);

    // --- Secuencia de Inicialización para HD44780 en 4-bit mode ---
    // (Referencia: Datasheet del HD44780 o similar)
    esp_rom_delay_us(20000); // Esperar más de 15ms después de VCC sube a 4.5V (o 20ms por seguridad)

    // 1. Resetear a modo 8-bit (parte 1 de 3)
    lcd_send_nibble(0x03);
    lcd_pulse_enable();
    esp_rom_delay_us(4500); // Esperar más de 4.1ms

    // 2. Resetear a modo 8-bit (parte 2 de 3)
    lcd_send_nibble(0x03);
    lcd_pulse_enable();
    esp_rom_delay_us(150); // Esperar más de 100us

    // 3. Resetear a modo 8-bit (parte 3 de 3)
    lcd_send_nibble(0x03);
    lcd_pulse_enable();
    esp_rom_delay_us(100); // Pequeño delay

    // 4. Establecer modo 4-bit
    lcd_send_nibble(0x02); // Enviar 0x2 para configurar 4-bit mode
    lcd_pulse_enable();
    esp_rom_delay_us(100); // Pequeño delay

    // 5. Configurar Función (Function Set): 4-bit, 2 líneas, 5x8 puntos
    lcd_send_command(0x28); // DL=0 (4-bit), N=1 (2 líneas), F=0 (5x8 puntos)
    esp_rom_delay_us(100);

    // 6. Display ON/OFF Control: Display ON, Cursor OFF, Blink OFF
    lcd_send_command(0x0C); // D=1 (Display ON), C=0 (Cursor OFF), B=0 (Blink OFF)
    esp_rom_delay_us(100);

    // 7. Limpiar Display (Clear Display)
    lcd_send_command(0x01); // Limpiar display
    esp_rom_delay_us(2000); // Este comando requiere un delay largo (aprox 1.52ms)

    // 8. Entry Mode Set: Increment cursor, No display shift
    lcd_send_command(0x06); // I/D=1 (Incrementa), S=0 (No shift)
    esp_rom_delay_us(100);

    ESP_LOGI(TAG, "LCD: Inicializacion completa.");
}

/**
 * @brief Limpia el contenido del display.
 */
void lcd_clear(void) {
    lcd_send_command(0x01); // Comando para limpiar display
    esp_rom_delay_us(2000); // Requiere un delay largo
}

/**
 * @brief Establece la posición del cursor en el display.
 * @param col Columna (0-7 para 8 caracteres).
 * @param row Fila (0 o 1 para 2 líneas).
 */
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t address;
    if (row == 0) {
        address = 0x00 + col;
    } else {
        address = 0x40 + col; // Dirección para la segunda línea
    }
    lcd_send_command(0x80 | address); // Comando para establecer dirección DDRAM
    esp_rom_delay_us(100);
}

/**
 * @brief Escribe una cadena de texto en la posición actual del cursor.
 * @param str La cadena de texto a escribir (máx. 8 caracteres por línea).
 */
void lcd_write_string(const char *str) {
    while (*str) {
        lcd_send_data(*str++);
        esp_rom_delay_us(100); // Pequeño delay entre caracteres
    }
}

/**
 * @brief Escribe un valor float en el LCD, formateado como string.
 * @param value El valor float a mostrar.
 * @param total_width El ancho total del campo (por ejemplo, 7 para 3.3f + punto).
 * @param precision_decimals La cantidad de decimales a mostrar.
 *
 * Esta función toma un float, lo convierte a string con un formato específico
 * y lo envía al LCD. Utiliza snprintf para evitar desbordamiento del buffer
 * y manejar la precisión de forma segura.
 */
void lcd_write_float(float value, int total_width, int precision_decimals) {
    char str_buffer[total_width + 1]; // +1 para el caracter nulo de terminación
    
    // snprintf formatea el float en un string.
    // "%*.*f" significa:
    // *1 = total_width (ancho total)
    // *2 = precision_decimals (precisión de decimales)
    // f = float
    snprintf(str_buffer, sizeof(str_buffer), "%*.*f", total_width, precision_decimals, value);

    // Escribir el string formateado en el LCD
    lcd_write_string(str_buffer);
}

/**
 * @brief Funcion para refrescar lo que se muestra en el LCD.
 * Basado en la variable global `current_selection`.
 * Muestra la opcion actual y la siguiente.
 */
static void refresh_lcd_display(void) {
    lcd_clear();
    char line_buffer[LCD_COLS + 1];

    if (current_state == MENU_STATE_WEIGHING) {
        lcd_set_cursor(0, 0);
        strncpy(line_buffer, "Peso..Kg", LCD_COLS);
        line_buffer[LCD_COLS] = '\0';
        lcd_write_string(line_buffer);
        
        // Formatear el peso para mostrar en la segunda linea
        lcd_set_cursor(0, 1);
        lcd_write_float(current_weight_display, 7, 2);
        
    }
    else if (current_state == MENU_STATE_TARAR) {
        lcd_set_cursor(0, 0);
        strncpy(line_buffer, "Tarar.Kg", LCD_COLS);
        line_buffer[LCD_COLS] = '\0';
        lcd_write_string(line_buffer);
        
        // Formatear el peso para mostrar en la segunda linea
        lcd_set_cursor(0, 1);
        lcd_write_float(current_weight_display, 7, 2);
        
    }
    else if (current_state == MENU_STATE_HEIGHT) {
        lcd_set_cursor(0, 0);
        strncpy(line_buffer, "Errores.", LCD_COLS);
        line_buffer[LCD_COLS] = '\0';
        lcd_write_string(line_buffer);
        
        // Formatear el peso para mostrar en la segunda linea
        lcd_set_cursor(0, 1);
        lcd_write_string(current_error_display); // Son 8 caracteres
        
    } 
    else if (current_state == MENU_STATE_ERRORS) {
        lcd_set_cursor(0, 0);
        strncpy(line_buffer, "Altura..", LCD_COLS);
        line_buffer[LCD_COLS] = '\0';
        lcd_write_string(line_buffer);
        
        // Formatear el peso para mostrar en la segunda linea
        lcd_set_cursor(0, 1);
        lcd_write_float(current_height_display, 7, 2);
        
    } 
    else if (current_state == MENU_STATE_NAVIGATING) { // MENU_STATE_NAVIGATING

        num_options = 0;
        if (current_menu_ptr == menu_options_balanza) {
            num_options = NUM_MENU_OPTIONS_BALANZA;
        } else if (current_menu_ptr == menu_options_inclinacion) {
            num_options = NUM_MENU_OPTIONS_INCLINACION;
        } else if (current_menu_ptr == menu_options_altura) {
            num_options = NUM_MENU_OPTIONS_ALTURA;
        } else if (current_menu_ptr == menu_options_barandas) {
            num_options = NUM_MENU_OPTIONS_BARANDAS;
        } else if (current_menu_ptr == menu_options_freno) {
            num_options = NUM_MENU_OPTIONS_FRENO;
        } else if (current_menu_ptr == menu_options_idle) {
            num_options = NUM_MENU_OPTIONS_IDLE;
        } else {
            num_options = NUM_MAIN_MENU_OPTIONS;
        }

        if (num_options > 0) {
            // Calcular el índice del elemento anterior de forma circular
            prev_selection_index = (current_selection - 1 + num_options) % num_options;
            
            // Muestra la opcion anterior en la Linea 1 sin cursor
            lcd_set_cursor(0, 0);
            lcd_write_string(current_menu_ptr[prev_selection_index].text);

            // Muestra la opcion seleccionada en la Linea 2 con el cursor
            lcd_set_cursor(0, 1);
            lcd_write_string(">");
            lcd_set_cursor(1, 1);
            lcd_write_string(current_menu_ptr[current_selection].text);
        } else {
            // En caso de que no haya opciones
            lcd_set_cursor(0, 0);
            lcd_write_string("Sin Menu");
        }
    }
}

/**
 * @brief Tarea de FreeRTOS para actualizar el display LCD.
 * Lee las cadenas globales y las muestra en el LCD.
 * @param pvParameters No usado.
 */
void lcd_display_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de display LCD iniciada.");

    lcd_init(); // Inicializar el LCD al inicio de la tarea
    
    // Mensaje de bienvenida inicial (se sobreescribirá pronto)
    lcd_set_cursor(0, 0);
    lcd_write_string("..POWER.");
    lcd_set_cursor(0, 1);
    lcd_write_string("...On...");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo

    refresh_lcd_display(); // Muestra el menu inicial

    button_event_t received_event;
    float received_weight;
    float received_height;
    char received_error[8];

    while (1) {
        if (current_state == MENU_STATE_NAVIGATING) {

            if (xQueueReceive(button_event_queue, &received_event, portMAX_DELAY) == pdPASS) {
                int num_options = 0;
                if (current_menu_ptr == menu_options_balanza) {
                    num_options = NUM_MENU_OPTIONS_BALANZA;
                } else if (current_menu_ptr == menu_options_inclinacion) {
                    num_options = NUM_MENU_OPTIONS_INCLINACION;
                } else if (current_menu_ptr == menu_options_altura) {
                    num_options = NUM_MENU_OPTIONS_ALTURA;
                } else if (current_menu_ptr == menu_options_barandas) {
                    num_options = NUM_MENU_OPTIONS_BARANDAS;
                } else if (current_menu_ptr == menu_options_freno) {
                    num_options = NUM_MENU_OPTIONS_FRENO;
                } else if (current_menu_ptr == menu_options_idle) {
                    num_options = NUM_MENU_OPTIONS_IDLE;
                } else {
                    num_options = NUM_MAIN_MENU_OPTIONS;
                }
                
                ESP_LOGD(TAG_MENU, "Evento de boton recibido, procesando...");
                
                switch (received_event) {
                    case EVENT_BUTTON_UP:
                        current_selection = (current_selection - 1 + num_options) % num_options;
                        break;
                    case EVENT_BUTTON_DOWN:
                        current_selection = (current_selection + 1) % num_options;
                        break;
                    case EVENT_BUTTON_SELECT:
                        // Obtener el item seleccionado
                        menu_item_t *selected_item = &current_menu_ptr[current_selection];

                        // Comprobar si tiene un submenú
                        if (selected_item->submenu) {
                            // Navegar al submenú
                            parent_menu_ptr = current_menu_ptr; // Guardar el menú actual como padre
                            current_menu_ptr = selected_item->submenu;
                            current_selection = 0; // Resetear la selección al entrar en un submenú
                        }
                        // Comprobar si es la opción "Atras"
                        else if (strcmp(selected_item->text, "Atras") == 0) {
                            if (parent_menu_ptr) {
                                current_menu_ptr = parent_menu_ptr; // Volver al menú padre
                                parent_menu_ptr = NULL; // El nuevo padre es el menú principal
                                current_selection = 0; // Resetear la selección
                            }
                        }
                        else if (strcmp(selected_item->text, "Pesar") == 0) {
                            ESP_LOGI(TAG_MENU, "Entrando en modo de pesaje...");
                            current_state = MENU_STATE_WEIGHING;
                            refresh_lcd_display();
                        }
                        else if (strcmp(selected_item->text, "Altura") == 0) {
                            ESP_LOGI(TAG_MENU, "Entrando en modo de altura...");
                            current_state = MENU_STATE_HEIGHT;
                            refresh_lcd_display();
                        } 
                        else if (strcmp(selected_item->text, "Tarar") == 0) {
                            ESP_LOGI(TAG_MENU, "Entrando en modo de tara...");
                            current_state = MENU_STATE_TARAR;
                            refresh_lcd_display();
                        } 
                        else if (selected_item->action) {
                            selected_item->action();
                        }
                        break;
                }
                refresh_lcd_display();
            }
        } else if (current_state == MENU_STATE_WEIGHING) {
            // Espera por un evento de la cola de botones o de la cola de peso.
            // No bloquea para siempre, si no hay nada en 100ms, refresca el display.
            if (xQueueReceive(weight_queue, &received_weight, pdMS_TO_TICKS(100)) == pdPASS) {
                current_weight_display = received_weight;
                refresh_lcd_display();
            }
            
            if (xQueueReceive(button_event_queue, &received_event, (TickType_t)0) == pdPASS && received_event == EVENT_BUTTON_SELECT) {
                ESP_LOGI(TAG_MENU, "Saliendo de modo de pesaje...");
                current_state = MENU_STATE_NAVIGATING;
                refresh_lcd_display(); // Refrescar la pantalla para mostrar el menu
            }
        } else if (current_state == MENU_STATE_TARAR) {
            // Espera por un evento de la cola de botones o de la cola de peso.
            // No bloquea para siempre, si no hay nada en 100ms, refresca el display.
            if (xQueueReceive(weight_queue, &received_weight, pdMS_TO_TICKS(100)) == pdPASS) {
                current_weight_display = received_weight;
                refresh_lcd_display();
            }
            
            if (xQueueReceive(button_event_queue, &received_event, (TickType_t)0) == pdPASS && received_event == EVENT_BUTTON_SELECT) {
                ESP_LOGI(TAG_MENU, "Saliendo de modo de pesaje...");
                current_state = MENU_STATE_NAVIGATING;
                refresh_lcd_display(); // Refrescar la pantalla para mostrar el menu
            }
        } else if (current_state == MENU_STATE_HEIGHT) {
            if (xQueueReceive(height_queue, &received_height, pdMS_TO_TICKS(100)) == pdPASS) {
                current_height_display = received_height;
                refresh_lcd_display();
            }
            
            if (xQueueReceive(button_event_queue, &received_event, (TickType_t)0) == pdPASS && received_event == EVENT_BUTTON_SELECT) {
                ESP_LOGI(TAG_MENU, "Saliendo de modo de pesaje...");
                current_state = MENU_STATE_NAVIGATING;
                refresh_lcd_display(); // Refrescar la pantalla para mostrar el menu
            }
        } else if (current_state == MENU_STATE_ERRORS) {
            if (xQueueReceive(error_queue, &received_error, pdMS_TO_TICKS(100)) == pdPASS) {
                strcpy(current_error_display,received_error);
                refresh_lcd_display();
            }
            
            if (xQueueReceive(button_event_queue, &received_event, (TickType_t)0) == pdPASS && received_event == EVENT_BUTTON_SELECT) {
                ESP_LOGI(TAG_MENU, "Saliendo de modo de pesaje...");
                current_state = MENU_STATE_NAVIGATING;
                refresh_lcd_display(); // Refrescar la pantalla para mostrar el menu
            }
        }
    }
}

// --- Implementaciones de funciones de accion (DUMMY) ---
static void balance_errors_action(void) { ESP_LOGI(TAG_MENU, "Accion: Errores Balanza"); }
static void inclination_head_action(void) { ESP_LOGI(TAG_MENU, "Accion: Mover cabecera"); }
static void inclination_foot_action(void) { ESP_LOGI(TAG_MENU, "Accion: Mover piecera"); }
static void inclination_errors_action(void) { ESP_LOGI(TAG_MENU, "Accion: Errores Inclinacion"); }
static void height_errors_action(void) { ESP_LOGI(TAG_MENU, "Accion: Errores Altura"); }
static void railings_status_action(void) { ESP_LOGI(TAG_MENU, "Accion: Estado Barandas"); }
static void railings_errors_action(void) { ESP_LOGI(TAG_MENU, "Accion: Errores Barandas"); }
static void brake_status_action(void) { ESP_LOGI(TAG_MENU, "Accion: Estado Freno"); }
static void brake_errors_action(void) { ESP_LOGI(TAG_MENU, "Accion: Errores Freno"); }
static void idle_000_action(void) { ESP_LOGI(TAG_MENU, "Accion: 000"); }
static void idle_001_action(void) { ESP_LOGI(TAG_MENU, "Accion: 001"); }
static void idle_010_action(void) { ESP_LOGI(TAG_MENU, "Accion: 010"); }
static void idle_011_action(void) { ESP_LOGI(TAG_MENU, "Accion: 011"); }
static void idle_100_action(void) { ESP_LOGI(TAG_MENU, "Accion: 100"); }

static void balance_weigh_action(void) { 
    ESP_LOGI(TAG_MENU, "Accion: Pesar");
}

static void height_status_action(void) { 
    ESP_LOGI(TAG_MENU, "Accion: Estado Altura"); 
}

static void balance_calibrate_action(void) { 
    ESP_LOGI(TAG_MENU, "Accion: Calibrar Balanza"); 
}

static void balance_tare_action(void) { 
    ESP_LOGI(TAG_MENU, "Accion: Tarar Balanza"); 
    balanza_tarar();
}

// // --- Función principal de la aplicación ESP-IDF ---
// void app_main(void) {
//     esp_log_level_set(TAG, ESP_LOG_INFO); // Configurar el nivel de log

//     // --- Configuración de GPIOs para el LCD ---
//     // NO SE HACE AQUÍ DIRECTAMENTE, YA SE HACE EN lcd_init()
//     // Esto es solo un recordatorio para evitar duplicar la configuración de los pines del LCD en app_main()
//     // Si tienes otros pines GPIO que inicializar que no sean del LCD, irían aquí.

//     // Crear la tarea para el display LCD
//     xTaskCreate(lcd_display_task, "LCD_DisplayTask", 4096, NULL, 5, NULL); // Pila de 4KB

//     ESP_LOGI(TAG, "Aplicacion iniciada. Las tareas de FreeRTOS estan ejecutandose.");
//     // app_main() debe terminar su ejecución aquí. Las tareas FreeRTOS se ejecutan de forma independiente.
// }
