// Author: Federico Cañete  
// Date: 09/06/2025

#include "inc/ili9340.h"
#include "inc/fontx.h"
#include "inc/display_tft.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_caps.h"
#include "inc/pinout.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <sys/stat.h>

#define BMP_HEADER_SIZE 54
#define OFFSET_DATOS_BMP 70  // Cambiar a 54 si no usaste mi script para convertir JPG a BMP

// Corrección de offset X para ILI9340
#define LCD_X_OFFSET 0

static FontxFile latin32fx[2];
static FontxFile ilgh24fx[2];
static FontxFile Cons32fx[2];


void init_spiffs(char * path) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = path,
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Error inicializando SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI("SPIFFS", "SPIFFS montado correctamente");
}

void lcdDrawBMP(TFT_t *dev, const char *filename, int x, int y)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        ESP_LOGE("BMP Draw", "No se pudo abrir %s", filename);
        return;
    }

    uint8_t header[54];
    fread(header, 1, sizeof(header), f);

    if (header[0] != 'B' || header[1] != 'M') {
        ESP_LOGE("BMP Draw", "Archivo no válido BMP");
        fclose(f);
        return;
    }

    uint32_t data_offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
    uint32_t width  = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    uint32_t height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    uint16_t bpp    = header[28] | (header[29] << 8);

    if (bpp != 16) {
        ESP_LOGE("BMP Draw", "Solo BMP de 16 bits");
        fclose(f);
        return;
    }

    int rowSize = (width * 2 + 3) & ~3;
    fseek(f, data_offset, SEEK_SET);

    // Cambiar la asignación de memoria para asegurar que el buffer sea un múltiplo de 4 bytes
    // El tamaño en bytes es width * 2. Aseguramos que sea el siguiente múltiplo de 4.
    size_t buffer_size_bytes = (width * 2 + 3) & ~3; // Siguiente múltiplo de 4

    uint16_t *line = heap_caps_aligned_alloc(4, buffer_size_bytes, MALLOC_CAP_DMA);
    /*
    // Alinear puntero a múltiplo de 4 bytes
    uintptr_t ptr = (uintptr_t)line;
    ptr = (ptr + 3) & ~3;
    line = (uint16_t *)ptr;
    */

    uint8_t  *row_buf = malloc(rowSize);
    if (!line || !row_buf) {
        ESP_LOGE("BMP Draw", "No hay memoria");
        if (line) free(line);
        if (row_buf) free(row_buf);
        fclose(f);
        return;
    }

    uint16_t x0 = x + dev->_offsetx + LCD_X_OFFSET;
    if (x0 < 0) x0 = 0;
    uint16_t y0 = y + dev->_offsety;
    uint16_t x1 = x0 + width  - 1;
    uint16_t y1 = y0 + height - 1;

    if (x1 >= dev->_width)  x1 = dev->_width  - 1;
    if (y1 >= dev->_height) y1 = dev->_height - 1;

    ESP_LOGI("BMP", "offset: %d", dev->_offsetx);
    ESP_LOGI("BMP", "Ventana: x=%d..%d y=%d..%d", x0, x1, y0, y1);

    // Configurar ventana una sola vez
    spi_master_write_comm_byte(dev, 0x2A); // Columna
    spi_master_write_addr(dev, x0, x1);
    spi_master_write_comm_byte(dev, 0x2B); // Fila
    spi_master_write_addr(dev, y0, y1);
    spi_master_write_comm_byte(dev, 0x2C); // Memory Write

    // Enviar cada fila (de abajo hacia arriba)
    for (int row = 0; row < height; row++) {
        fseek(f, data_offset + (height - 1 - row) * rowSize, SEEK_SET);
        fread(row_buf, 1, rowSize, f);
        memcpy(line, row_buf, width * 2);

        // 🔄 Byte-swap - Convertir de little endian (BMP) a big endian (LCD)
        for (int i = 0; i < width; i++) {
            uint16_t c = line[i];
            line[i] = (c << 8) | (c >> 8);
        }

        /*

        // ⚙️ Alinear a múltiplo de 4 bytes
        size_t tx_len = (width * 2 + 3) & ~3;

        spi_transaction_t t = {0};
        t.length = tx_len * 8;  // en bits
        t.tx_buffer = line;
        gpio_set_level(dev->_dc, 1);*/

        // 🧩 Importante: usar transmit bloqueante para estabilidad
        //spi_device_transmit(dev->_TFT_Handle, &t);
        spi_master_write_colors_fast(dev, line, width);
    }

    free(line);
    free(row_buf);
    fclose(f);
    ESP_LOGI("BMP", "Imagen renderizada correctamente");
}


void display_tft_task(void *pvParameters) {

    TFT_t dev;
    //Dentro de dev hay un campo _TFT_Handle que es el handle del dispositivo SPI (spi_device_handle_t)
    //ese mismo lo usaré para el DMA
    
    InitFontx(latin32fx, "/data/LATIN32B.FNT", "");
    InitFontx(ilgh24fx, "/data/ILGH24XB.FNT", "");
    InitFontx(Cons32fx, "/data/Cons32.FNT", "");
    //Recordar que cada vez que se cargue una fuente nueva, hay que grabarla en memoria: pio run -t uploadfs

    uint16_t contador = 2;
    char string_contador[5];


    uint16_t model = 0x9341; //ILI9341
    ESP_LOGI("TFT", "Disabling Touch Contoller");
	int XPT_MISO_GPIO = -1;
	int XPT_CS_GPIO = -1;
	int XPT_IRQ_GPIO = -1;
	int XPT_SCLK_GPIO = -1;
	int XPT_MOSI_GPIO = -1;

    init_spiffs("/data");

    spi_clock_speed(1*1000*1000); // 1 MHz
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
		CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);

    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

    //VER
    //lcdSetRotation(&dev, 1);

    //. 

    lcdFillScreen(&dev, WHITE);
    lcdSetFontDirection(&dev, DEFAULT_ORIENTATION);
    
    // Abrir JPG desde SPIFFS

    struct stat st;
    if (stat("/data/tony.bmp", &st) == 0) {
        ESP_LOGI("FILE", "Tamaño del archivo %s: %ld bytes", "/data/tony.bmp", st.st_size);
    } else {
        ESP_LOGE("FILE", "No se puede acceder al archivo %s", "/data/tony.bmp");
    }

    lcdDrawBMP(&dev, "/data/linea.bmp", 0, 0);

    vTaskDelay(pdMS_TO_TICKS(20*10*100));  

    lcdDrawFillRect(&dev, 0, 0, 238, 319, RED);
    vTaskDelay(pdMS_TO_TICKS(5*10*100));  


    // Dibujar un rectángulo rojo 100x100 manualmente
    uint16_t lines[100];

    // Rellenar línea roja (byte-swapped)
    for (int i = 0; i < 100; i++) {
        uint16_t c = YELLOW;
        lines[i] = (c << 8) | (c >> 8);   // 🔄 swap bytes
    }

    lcdSetWindow(&dev, 0, 0, 99, 99);
    spi_master_write_comm_byte(&dev, 0x2C);

    for (int y = 0; y < 100; y++) {
        spi_master_write_colors_fast(&dev, lines, 100);
    }

    vTaskDelay(pdMS_TO_TICKS(5*10*100));  


    lcdFillScreen(&dev, FONDO_BIENVENIDA);
    lcdDrawString(&dev, Cons32fx, 60, 240, (uint8_t *)"BIENVENIDO", BLACK);
    lcdDrawString(&dev, Cons32fx, 160, 220, (uint8_t *)"Kit AC", AZUL_OCEANO);
    vTaskDelay(pdMS_TO_TICKS(5000));  
    
    //Borrar textos
    lcdDrawFillRect(&dev, 35, 80, 60, 240, FONDO_BIENVENIDA);
    lcdDrawFillRect(&dev, 135, 120, 160, 220, FONDO_BIENVENIDA);

    //Pantalla Menu Principal
    lcdDrawString(&dev, ilgh24fx, 35, 244, (uint8_t *)"Menu Principal", BLACK);
    lcdDrawLine(&dev, 50, 0, 50, 320, GRAY);
    lcdDrawFillRect(&dev, 0, 0, 50, 56, GRAY);
    lcdDrawFillRect(&dev, 0, 260, 50, 320, GRAY);
    
    lcdDrawString(&dev, ilgh24fx, 84, 290, (uint8_t *)"1. Estado general", AZUL_OCEANO);
    lcdDrawString(&dev, ilgh24fx, 118, 290, (uint8_t *)" > Peso del paciente", RED);
    lcdDrawString(&dev, ilgh24fx, 152, 290, (uint8_t *)"3. Altura e inclinacion", AZUL_OCEANO);
    lcdDrawString(&dev, ilgh24fx, 186, 290, (uint8_t *)"4. Barandal y freno", AZUL_OCEANO);
    lcdDrawString(&dev, ilgh24fx, 220, 290, (uint8_t *)"5. Configuracion", AZUL_OCEANO);
    vTaskDelay(pdMS_TO_TICKS(4000));  


    //Borrar textos
    lcdDrawFillRect(&dev, 60, 0, 220, 290, FONDO_BIENVENIDA);
    lcdDrawFillRect(&dev, 0, 46, 49, 270, FONDO_BIENVENIDA);

    //Pantalla Peso del paciente
    lcdDrawString(&dev, ilgh24fx, 35, 260, (uint8_t *)"Peso del paciente", BLACK);
    lcdDrawString(&dev, Cons32fx, 102, 216, (uint8_t *)"85,7 kg", RED);
    lcdDrawLine(&dev, 107,104,107, 216, BLUE);
    lcdDrawLine(&dev, 108,104,108, 216, BLUE);

    lcdDrawString(&dev, ilgh24fx, 152, 300, (uint8_t *)"Ultimo peso: 87,0 kg", BLACK);
    lcdDrawString(&dev, ilgh24fx, 225, 300, (uint8_t *)"Recalibrar", BLACK);
    lcdDrawRect(&dev, 196, 175, 230, 305, GRAY);
    lcdDrawString(&dev, ilgh24fx, 225, 90, (uint8_t *)"Volver", BLACK);



    while (1) {

        /*    
        lcdDrawFillRect(&dev, 55, 240, 75, 300, WHITE);
        sprintf(string_contador, "%d", contador);   
        lcdDrawString(&dev, Cons32fx, 80, 300, (uint8_t *)&string_contador, RED);    
        vTaskDelay(pdMS_TO_TICKS(1000));  
        contador*=2;  
        if (contador > 900) {
            contador = 1;
            lcdFillScreen(&dev, WHITE);
            lcdDrawString(&dev, ilgh24fx, 40, 300, (uint8_t *)"Muestro variables:", GREEN);
        }
        */
        vTaskDelay(pdMS_TO_TICKS(1000));  

    }

}


