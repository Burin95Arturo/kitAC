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
#include "inc/tjpgd.h"

#include "esp_spiffs.h"

static FontxFile latin32fx[2];
static FontxFile ilgh24fx[2];
static FontxFile Cons32fx[2];

// Callback para leer bytes desde archivo
static size_t input_func(JDEC *jd, uint8_t *buff, size_t nbyte) {
    FILE *fp = (FILE *)jd->device;
    if (buff) {
        return fread(buff, 1, nbyte, fp);
    } else {
        fseek(fp, nbyte, SEEK_CUR);
        return nbyte;
    }
}

// Callback para dibujar los bloques decodificados. Usa DMA (encola comando/ventana + encola datos y espera) */
static int output_func(JDEC *jd, void *bitmap, JRECT *rect) {

    // bitmap es un buffer con pixels en RGB565 (uint16_t) provisto por TJpgDec
    TFT_t *dev = (TFT_t *)jd->device;
    TFT_t *dummy = NULL; // no usamos la abstracción del driver; usamos spi_dev
    (void) dummy;

    int x0 = rect->left;
    int y0 = rect->top;
    int x1 = rect->right;
    int y1 = rect->bottom;

    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;

    // 1) set window (encola comandos y datos de ventana)
    if (ili9341_set_window_dma(dev->_TFT_Handle, x0, y0, x1, y1) != ESP_OK) {
        ESP_LOGE("output_func", "set_window DMA failed");
        return 0;
    }
    ESP_LOGI("output", "Seteo ventana");


    // 2) enviar los pixels decodificados como una transacción DMA
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = w * h * 16;     // bits (RGB565 = 16 bits)
    t.tx_buffer = bitmap;
    t.user = (void*)1;         // data

    // Encolar y esperar (para simplicidad y orden)
    esp_err_t r = queue_trans_and_wait(dev->_TFT_Handle, &t);
    if (r != ESP_OK) {
        ESP_LOGE("output_func", "queue data trans failed: %d", r);
        return 0;
    }

    // Además necesitamos esperar a que terminen las transacciones de los comandos previos:
    // Los comandos y datos fueron encolados; como hacemos get_trans_result de la transacción de datos,
    // las previas ya se ejecutaron (SPI ejecuta en orden).
    return 1;
    
/*ORIGINAL*/    
/*
    TFT_t *dev = (TFT_t *)jd->device;
    uint16_t *pixels = (uint16_t *)bitmap;
    int w = rect->right - rect->left + 1;
    int h = rect->bottom - rect->top + 1;

    // Dibuja cada píxel en el LCD
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t color = pixels[y * w + x];
            lcdDrawPixel(dev, rect->left + x, rect->top + y, color);
        }
    }
    return 1;*/
}

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

    spi_clock_speed(8*1000*1000); // 8 MHz
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
		CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);

    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
    //VER
    //lcdSetRotation(&dev, 1);


    lcdFillScreen(&dev, WHITE);
    lcdSetFontDirection(&dev, DEFAULT_ORIENTATION);
    
    // Abrir JPG desde SPIFFS
    FILE *fp = fopen("/data/logo.jpg", "rb");

    
    if (fp) 
    {
        // 6) Preparar TJpgDec con work en memoria DMA-capable
        JDEC jd;
        void *work = heap_caps_malloc(4096, MALLOC_CAP_DMA);  // work = buffer de trabajo via DMA
        
        if (work) {
                        
            JRESULT res = jd_prepare(&jd, input_func, work, 4096, (void *)fp);

            if (res == JDR_OK) {
              
                // 7) Decodificar (output_func_dma usa cola DMA y get_trans_result)
                // Importante: en el callback se usará la variable global dev
                jd.device = &dev;
                // Decodificar → llama a output_func() para cada bloque
                res = jd_decomp(&jd, output_func, 0);  // 0 = tamaño original

                if (res == JDR_OK) {
                    ESP_LOGI("TFT", "JPEG decodificado OK");
                } else {
                    ESP_LOGE("TFT", "jd_decomp error %d", res);
                }

            } else {
                ESP_LOGE("TFT", "jd_prepare error %d", res);
            }
            fclose(fp);
            free(work);
           
        } else {
            ESP_LOGE("TFT", "No hay RAM para buffer");
            fclose(fp);
        }

    } else {
        ESP_LOGE("TFT", "No se pudo abrir HOLA.jpg");
    }
    
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
    lcdDrawString(&dev, Cons32fx, 102, 216, (uint8_t *)"85,7 kg", BLUE);
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