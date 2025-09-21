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

static FontxFile fx[2];


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
    InitFontx(fx, "/data/LATIN32B.FNT", "");

    uint16_t model = 0x9341; //ILI9341
    ESP_LOGI("TFT", "Disabling Touch Contoller");
	int XPT_MISO_GPIO = -1;
	int XPT_CS_GPIO = -1;
	int XPT_IRQ_GPIO = -1;
	int XPT_SCLK_GPIO = -1;
	int XPT_MOSI_GPIO = -1;

    init_spiffs("/data");

    spi_clock_speed(5*1000*1000); // 5 MHz
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
		CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);

    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
    //VER
    //lcdSetRotation(&dev, 1);


    lcdFillScreen(&dev, RED);
    lcdDrawString(&dev, fx, 20, 50, (uint8_t *)"Hola!", BLACK);
    
    while (1) {

        vTaskDelay(pdMS_TO_TICKS(100));
        lcdDrawString(&dev, fx, 20, 102, (uint8_t *)"Todo bien?", BLACK);
        vTaskDelay(pdMS_TO_TICKS(3000));
        lcdFillScreen(&dev, PURPLE);
    }

}