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
// Definir una compensación que sea un múltiplo seguro de 4 bytes.
// Usaremos 16 bytes (el siguiente múltiplo de 4 bytes después de los 12 problemáticos).
#define DMA_SAFE_OFFSET_BYTES 16 // 8 uint16_t

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

// Mostrar imagen BMP 16-bit (RGB565) - Optimizado con MADCTL
//Para el display en horizontal, el offset es así:
//X = Vertical
//Y = Horizontal
void lcdDrawBMP(TFT_t *dev, const char *filename, uint16_t y, uint16_t x)
{
    // ... (Código de apertura y verificación de headers OMITIDO por brevedad, es el mismo) ...

    FILE *f = fopen(filename, "rb");
    if (!f) { /* ... handle error ... */ return; }

    uint8_t header[54];
    if (fread(header, 1, 54, f) != 54) { /* ... handle error ... */ fclose(f); return; }
    if (header[0] != 'B' || header[1] != 'M') { /* ... handle error ... */ fclose(f); return; }

    uint32_t data_offset = *(uint32_t *)&header[10];
    int32_t width  = *(int32_t *)&header[18];
    int32_t height = *(int32_t *)&header[22];
    uint16_t bpp = *(uint16_t *)&header[28];
    int32_t absHeight = (height < 0) ? -height : height;

    if (bpp != 16) { /* ... handle error ... */ fclose(f); return; }

    // Cálculo correcto del stride (ancho de fila en bytes con padding)
    int rowSize = ((width * 2 + 3) & ~3);

    // --- 1. CAMBIO CRÍTICO: Modificar MADCTL ---
    spi_master_write_comm_byte(dev, 0x36); // Memory Access Control
    spi_master_write_data_byte(dev, 0x88); // 0x08 (Original) | MY_BIT (0x80) = 0x88
    
    // Configurar ventana en el LCD
    uint16_t x0 = x + dev->_offsetx;
    uint16_t y0 = y + dev->_offsety;
    uint16_t x1 = x0 + width - 1;
    uint16_t y1 = y0 + absHeight - 1;

    spi_master_write_comm_byte(dev, 0x2A); 
    spi_master_write_addr(dev, x0, x1);
    spi_master_write_comm_byte(dev, 0x2B); 
    spi_master_write_addr(dev, y0, y1);
    spi_master_write_comm_byte(dev, 0x2C); 

    // Buffers DMA-safe
    uint16_t *line = heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_DMA);
    uint8_t *row_buf = malloc(rowSize); // Buffer de lectura cruda

    if (!line || !row_buf) { 
        // ... (handle memory error, clean up, and restore MADCTL)
        spi_master_write_comm_byte(dev, 0x36);
        spi_master_write_data_byte(dev, 0x08);
        return;
    }
    
    // --- 2. POSICIONAMIENTO INICIAL ---
    // Mover el puntero del archivo al inicio de los datos de píxeles
    fseek(f, data_offset, SEEK_SET);

    // --- 3. LECTURA LINEAL (NO MÁS FSEEK DENTRO DEL BUCLE) ---
    for (int row = 0; row < absHeight; row++) {
        
        // El puntero del archivo avanza automáticamente
        size_t bytesRead = fread(row_buf, 1, rowSize, f);
        bool readSuccess = (bytesRead >= width * 2); // Éxito si leímos los datos útiles

        if (readSuccess) {
            // Conversión Little Endian (BMP) a Big Endian (Display)
            uint16_t *src = (uint16_t *)row_buf;
            for (int i = 0; i < width; i++) {
                uint16_t pixel = src[i];
                line[i] = (pixel << 8) | (pixel >> 8); 
            }
        } else {
            // Rellenar con NEGRO para no desincronizar la GRAM
            memset(line, 0, width * sizeof(uint16_t)); 
        }

        // Transmisión obligatoria de la fila
        spi_transaction_t t = {0};
        t.length = width * 16; 
        t.tx_buffer = line;
        gpio_set_level(dev->_dc, 1);
        spi_device_transmit(dev->_TFT_Handle, &t);
    }
    
    // --- 4. RESTAURACIÓN DEL MADCTL ---
    spi_master_write_comm_byte(dev, 0x36);
    spi_master_write_data_byte(dev, 0x08); // Vuelve al valor original
    
    // Limpieza final
    free(line);
    free(row_buf);
    fclose(f);
}

void display_tft_task(void *pvParameters) {

    //static display_t received_display_data;

    
    TFT_t dev;
    //Dentro de dev hay un campo _TFT_Handle que es el handle del dispositivo SPI (spi_device_handle_t)
    //ese mismo lo usaré para el DMA
    
    //Fuentes
    InitFontx(latin32fx, "/data/LATIN32B.FNT", "");
    InitFontx(ilgh24fx, "/data/ILGH24XB.FNT", "");
    InitFontx(Cons32fx, "/data/Cons32.FNT", "");
    //Recordar que cada vez que se cargue una fuente nueva, hay que grabarla en memoria: pio run -t uploadfs

    uint16_t angulo = 37;
    char string_angulo[4];

    uint16_t model = 0x9341; //ILI9341
    ESP_LOGI("TFT", "Disabling Touch Contoller");
	int XPT_MISO_GPIO = -1;
	int XPT_CS_GPIO = -1;
	int XPT_IRQ_GPIO = -1;
	int XPT_SCLK_GPIO = -1;
	int XPT_MOSI_GPIO = -1;


    //Inicialización SPI y TFT
    spi_clock_speed(10*1000*1000); // 10 MHz
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
		CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);

    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

    //VER
    //lcdSetRotation(&dev, 1);

    lcdFillScreen(&dev, WHITE);
    lcdSetFontDirection(&dev, DEFAULT_ORIENTATION);

    //Necesario para las fuentes e imágenes desde SPIFFS
    init_spiffs("/data");
    
    /* SECCIÓN PARA MANEJO DE IMÁGENES */
    /************************************************* */ 
    // Abrir JPG desde SPIFFS
    
        //Testeo de integridad del archivo
        /*
        struct stat st;
        if (stat("/data/tony189.bmp", &st) == 0) {
            ESP_LOGI("FILE", "Tamaño del archivo %s: %ld bytes", "/data/tony189.bmp", st.st_size);
        } else {
            ESP_LOGE("FILE", "No se puede acceder al archivo %s", "/data/tony189.bmp");
        }
        */

    lcdDrawBMP(&dev, "/data/kitac_logo.bmp", 99, 30);
    lcdDrawBMP(&dev, "/data/utn_logo.bmp", 228, 207);

    vTaskDelay(pdMS_TO_TICKS(3000));  

    lcdFillScreen(&dev, WHITE);

        //Testeo de dibujo rápido con DMA
            /*// Dibujar un rectángulo rojo 100x100 manualmente
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
            vTaskDelay(pdMS_TO_TICKS(1000));  
            */

    /************************************************* */ 

    //Pantalla Inicial
        //Barra superior de indicación de freno    
        lcdDrawFillRect(&dev, 0, 0, 44, 320, VERDE_TEMA);
        lcdDrawString(&dev, ilgh24fx, 34, 220, (uint8_t *)"FRENO OK", WHITE);

        //lcdDrawString(&dev, ilgh24fx, 80, 110, (uint8_t *)"Cabecera", BLACK);
        //Iconito de grados
        lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)"37", AZUL_OCEANO);
        lcdDrawFillCircle(&dev, 85, 127, 3, AZUL_OCEANO);
            lcdDrawBMP(&dev, "/data/inclinacion.bmp", 40, 65);

        //Iconito de altura
        lcdDrawString(&dev, Cons32fx, 155, 180, (uint8_t *)"POSICION", GREEN);
        lcdDrawString(&dev, Cons32fx, 180, 165, (uint8_t *)"SEGURA", GREEN);
            lcdDrawBMP(&dev, "/data/altura.bmp", 60, 125);
            lcdDrawBMP(&dev, "/data/tick.bmp", 20, 140);


        //Barra inferior con opciones
        lcdDrawFillRect(&dev, 196, 0, 239, 319, CELESTITO);

            lcdDrawFillRect(&dev, 207, 283, 229, 306, AZUL_OCEANO);
            lcdDrawRect(&dev, 230, 307, 206, 282, BLACK);
        lcdDrawString(&dev, ilgh24fx, 230, 300, (uint8_t *)"1", WHITE);

        lcdDrawString(&dev, ilgh24fx, 230, 270, (uint8_t *)"Balanza", BLACK);
        
        lcdDrawFillRect(&dev, 207, 103, 229, 126, AZUL_OCEANO);
        lcdDrawRect(&dev, 230, 127, 206, 104, BLACK);
        lcdDrawString(&dev, ilgh24fx, 230, 120, (uint8_t *)"2", WHITE);

        lcdDrawString(&dev, ilgh24fx, 230, 95, (uint8_t *)"Apagar", BLACK);

        vTaskDelay(pdMS_TO_TICKS(5000));  
        lcdDrawFillRect(&dev, 0, 0, 44, 320, RED);
        lcdDrawString(&dev, ilgh24fx, 34, 235, (uint8_t *)"REVISAR FRENO", WHITE);
        vTaskDelay(pdMS_TO_TICKS(5000));
        lcdDrawFillRect(&dev, 0, 0, 44, 320, VERDE_TEMA);
        lcdDrawString(&dev, ilgh24fx, 34, 220, (uint8_t *)"FRENO OK", WHITE);
   
    //Mover variables
    lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)"37", WHITE);
    sprintf(string_angulo, "%d", angulo);   

    for(angulo=37; angulo<86; angulo++) {
        lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)&string_angulo, WHITE);  
        sprintf(string_angulo, "%d", angulo);   
        lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)&string_angulo, AZUL_OCEANO);    
        vTaskDelay(pdMS_TO_TICKS(250));  

    }



    while (1) {

        // Leer de la cola central_queue
       // if (xQueueReceive(display_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
            // Procesar los datos recibidos
         //   ESP_LOGI("TFT_TASK", "Datos recibidos: %d", received_data);
            // Aquí puedes actualizar la pantalla TFT según los datos recibidos
        //}   

        
        
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
           //Barra superior de indicación de freno    
        lcdDrawFillRect(&dev, 0, 0, 44, 320, VERDE_TEMA);
        lcdDrawString(&dev, ilgh24fx, 34, 220, (uint8_t *)"FRENO OK", WHITE);
        vTaskDelay(pdMS_TO_TICKS(5000));
        lcdDrawFillRect(&dev, 0, 0, 44, 320, RED);
        lcdDrawString(&dev, ilgh24fx, 34, 235, (uint8_t *)"REVISAR FRENO", WHITE);
        vTaskDelay(pdMS_TO_TICKS(5000));


    }

}


