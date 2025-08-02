// Author: Burin Arturo
// Date: 20/06/2025

#include "inc/display_lcd.h"


// --- Definiciones para el Display ---
#define LCD_COLS 8 // Número de columnas (caracteres por línea)
#define LCD_ROWS 2 // Número de filas (líneas)

// --- TAG para los logs ---
static const char *TAG = "LCD_DISPLAY";

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

// // --- Prototipos de funciones públicas del driver LCD ---
// void lcd_init(void);
// void lcd_clear(void);
// void lcd_set_cursor(uint8_t col, uint8_t row);
// void lcd_write_string(const char *str);

// --- Tarea de FreeRTOS para el Display ---
// void lcd_display_task(void *pvParameters);


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
 * @brief Tarea de FreeRTOS para actualizar el display LCD.
 * Lee las cadenas globales y las muestra en el LCD.
 * @param pvParameters No usado.
 */
void lcd_display_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de display LCD iniciada.");

    lcd_init(); // Inicializar el LCD al inicio de la tarea
    
    // Mensaje de bienvenida inicial (se sobreescribirá pronto)
    lcd_set_cursor(0, 0);
    lcd_write_string("INICIAND");
    lcd_set_cursor(0, 1);
    lcd_write_string("O       ");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Esperar 1 segundo

    while (1) {
        // Actualizar línea 1
        // lcd_set_cursor(0, 0);
        // char temp_line1[LCD_COLS + 1];
        // strncpy(temp_line1, g_lcd_line1, LCD_COLS);
        // temp_line1[LCD_COLS] = '\0'; // Asegurar terminación nula
        // lcd_write_string(temp_line1);

        // // Actualizar línea 2
        // lcd_set_cursor(0, 1);
        // char temp_line2[LCD_COLS + 1];
        // strncpy(temp_line2, g_lcd_line2, LCD_COLS);
        // temp_line2[LCD_COLS] = '\0'; // Asegurar terminación nula
        // lcd_write_string(temp_line2);

        // ESP_LOGD(TAG, "LCD actualizado: '%s' | '%s'", g_lcd_line1, g_lcd_line2);
        

        // Delay de actualización del display. Ajusta según tus necesidades.
        // Un delay de 100ms a 500ms suele ser suficiente para actualizaciones visibles.
        vTaskDelay(pdMS_TO_TICKS(250));
    }
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
