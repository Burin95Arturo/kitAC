// Author: Federico Cañete  
// Date: 09/06/2025

//////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// Please update the following configuration according to your HardWare spec /////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LCD_HOST    SPI3_HOST
#define CONFIG_LCD_TYPE_ILI9341 1

#define PIN_NUM_MISO 19
#define CONFIG_MOSI_GPIO 23
#define CONFIG_SCLK_GPIO  18
#define CONFIG_TFT_CS_GPIO   5

#define CONFIG_DC_GPIO   21 //Data/Command control pin
#define CONFIG_RESET_GPIO  33
#define CONFIG_BL_GPIO 27

#define LCD_BK_LIGHT_ON_LEVEL   1

void init_spiffs(char * path);
void display_tft_task(void *pvParameters);

#define DEFAULT_ORIENTATION DIRECTION270 //Para verlo horizontal