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
#include "math.h"

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

static display_t received_data;
pantallas_t pantalla_actual = BIENVENIDA;


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

    int8_t angulo = 37;
    int8_t angulo_prev=37;
    char string_angulo[5];

    float peso = 55.5;
    float peso_prev = 55.5;
    char string_peso[8]; //Espacio para poder poner "-100.00" + NULL

    bool barandales_arriba = true;
    bool barandales_arriba_prev = true;
    char string_barandales[7];

    bool freno_ok = true;
    bool freno_ok_prev = true;
    char string_freno[4];

    uint8_t teclado_num = 1;
    uint8_t teclado_num_prev = 1;
    char string_teclado[4];

    uint8_t altura = 70;
    uint8_t altura_prev = 33;
    char string_altura[4];

    uint8_t contador_vueltas = 0;
    bool flag_refresh_display_data = true;


    uint16_t model = 0x9341; //ILI9341
    ESP_LOGI("TFT", "Disabling Touch Contoller");
	int XPT_MISO_GPIO = -1;
	int XPT_CS_GPIO = -1;
	int XPT_IRQ_GPIO = -1;
	int XPT_SCLK_GPIO = -1;
	int XPT_MOSI_GPIO = -1;

    vTaskDelay(pdMS_TO_TICKS(1000)); //Esperar a que se estabilice todo
    
    //--------Inicialización SPI y TFT--------//
    spi_clock_speed(10*1000*1000); // 10 MHz
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
		CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);
    lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
    lcdFillScreen(&dev, WHITE);
    lcdSetFontDirection(&dev, DEFAULT_ORIENTATION);
    //Necesario para las fuentes e imágenes desde SPIFFS
    init_spiffs("/data");
    //---------------------------------------//
    
    //--------Pantalla de Bienvenida--------//

    lcdDrawBMP(&dev, "/data/kitac_logo.bmp", 99, 30);
    lcdDrawBMP(&dev, "/data/utn_logo.bmp", 228, 207);
    vTaskDelay(pdMS_TO_TICKS(3000));  

    lcdFillScreen(&dev, WHITE);
    /******************************************/ 


    while (1) {

        // Leer de la cola central_queue
        if (xQueueReceive(display_queue, &received_data, pdMS_TO_TICKS(100)) == pdPASS) {
        
            //Según qué pantalla mostrar, actualizar datos
                        
            /*ESP_LOGI("TFT_TASK", "[Central] Origen: %d, Altura: %ld, Peso: %.2f, HALL_OnOff: %d, Freno: %d, Inclinacion: %f\n",
                received_data.data.origen,
                received_data.data.altura,
                received_data.data.peso,
                received_data.data.hall_on_off,
                received_data.data.freno_on_off,
                received_data.data.inclinacion);  */ 
            // Aquí puedes actualizar la pantalla TFT según los datos recibidos
        
            switch (received_data.pantalla)
            {
            case TESTS:
                /* code */
                if (pantalla_actual != TESTS) {
                    pantalla_actual = TESTS;
                    //Dibujar campos pantalla TESTS completa//
                    //--------Campos Pantalla TESTS--------//
                        lcdDrawFillRect(&dev, 45, 0, 239, 319, WHITE);
                        lcdDrawBMP(&dev, "/data/tonito.bmp", 244, 151);

                        //Barra superior con título 
                        lcdDrawFillRect(&dev, 0, 0, 44, 320, CELESTITO);
                        lcdDrawString(&dev, ilgh24fx, 34, 200, (uint8_t *)"TESTS", WHITE);

                        //INCLINACIÓN
                        lcdDrawString(&dev, ilgh24fx, 70, 295, (uint8_t *)"INCLINACION:", AZUL_OCEANO);
                        lcdDrawFillCircle(&dev, 53, 112, 3, AZUL_OCEANO);
                        sprintf(string_angulo, "%d", angulo_prev); //Sólo para inicializar

                        //BALANZA
                        lcdDrawString(&dev, ilgh24fx, 100, 295, (uint8_t *)"BALANZA:", AZUL_OCEANO);
                        lcdDrawString(&dev, ilgh24fx, 100, 103, (uint8_t *)"kg", VERDE_OSCURO);


                        //BARANDALES
                        lcdDrawString(&dev, ilgh24fx, 130, 295, (uint8_t *)"BARANDALES:", AZUL_OCEANO);
                        strcpy(string_barandales, "ARRIBA"); //Sólo para inicializar

                        //FRENO
                        lcdDrawString(&dev, ilgh24fx, 160, 295, (uint8_t *)"FRENO:", AZUL_OCEANO);
                        strcpy(string_freno, "OK"); //Sólo para inicializar


                        //TECLADO
                        lcdDrawString(&dev, ilgh24fx, 190, 295, (uint8_t *)"TECLADO:", AZUL_OCEANO);
                        sprintf(string_teclado, "%d", teclado_num); //Sólo para inicializar


                        //ALTURA
                        lcdDrawString(&dev, ilgh24fx, 220, 295, (uint8_t *)"ALTURA:", AZUL_OCEANO);
                        lcdDrawString(&dev, Cons32fx, 222, 150, (uint8_t *)"cm", BLACK);
                    //--------Fin campos Pantalla TESTS--------//
                }
                
                //--------Actualizar datos Pantalla TESTS--------//
                
                //Solo si el flag está activo, y poner filtro ORIGEN

                if (received_data.contains_data){

                    switch (received_data.data.origen)
                    {
                        case SENSOR_ACELEROMETRO:
                        //INCLINACIÓN
                        angulo = (u_int8_t)round(received_data.data.inclinacion);
                        if (angulo < 0) angulo = angulo * -1; //Valor absoluto
                        if (angulo != angulo_prev) {
                            lcdDrawString(&dev, Cons32fx, 72, 150, (uint8_t *)&string_angulo, WHITE);
                            sprintf(string_angulo, "%d", angulo);
                            lcdDrawString(&dev, Cons32fx, 72, 150, (uint8_t *)&string_angulo, AZUL_OCEANO);
                            angulo_prev = angulo;
                        }
                        break;

                        case CALCULO_PESO: 
                        //BALANZA
                        if (flag_refresh_display_data) {
                            lcdDrawFillRect(&dev, 77,106,96,184, WHITE);
                            sprintf(string_peso, "%.2f", received_data.data.peso_total);
                            lcdDrawString(&dev, Cons32fx, 102, 185, (uint8_t *)&string_peso, VERDE_OSCURO);
                            flag_refresh_display_data = false; //Este flag es para que no se refresque todo el tiempo el peso
                        }
                        break;

                        case SENSOR_HALL:
                       //BARANDALES
                        barandales_arriba = received_data.data.hall_on_off; 
                        if (barandales_arriba != barandales_arriba_prev) {
                            lcdDrawString(&dev, Cons32fx, 132, 160, (uint8_t *)string_barandales, WHITE);
                            if (barandales_arriba) {
                                strcpy(string_barandales, "ARRIBA");
                                lcdDrawString(&dev, Cons32fx, 132, 160, (uint8_t *)string_barandales, GREEN);
                            } else {
                                strcpy(string_barandales, "ABAJO");
                                lcdDrawString(&dev, Cons32fx, 132, 160, (uint8_t *)string_barandales, RED);
                            }
                            barandales_arriba_prev = barandales_arriba;
                        }
                        break;

                        case SENSOR_FRENO:
                        //FRENO
                        freno_ok = received_data.data.freno_on_off;
                        if (freno_ok != freno_ok_prev) {
                            lcdDrawString(&dev, Cons32fx, 162, 200, (uint8_t *)string_freno, WHITE);
                            if (freno_ok) {
                                strcpy(string_freno, "OK");
                                lcdDrawString(&dev, Cons32fx, 162, 200, (uint8_t *)string_freno, GREEN);
                            } else {
                                strcpy(string_freno, "NO");
                                lcdDrawString(&dev, Cons32fx, 162, 200, (uint8_t *)string_freno, RED);
                            }
                            freno_ok_prev = freno_ok;
                        }

                        case SENSOR_ALTURA:
                        //ALTURA
                        altura = (u_int8_t)round(received_data.data.altura);
                        if (altura != altura_prev) {
                            lcdDrawFillRect(&dev, 197, 154, 216, 198, WHITE);
                            sprintf(string_altura, "%d", altura);   
                            lcdDrawString(&dev, Cons32fx, 222, 200, (uint8_t *)&string_altura, BLACK);
                            altura_prev = altura;        
                        }

                        break;

                        case BUTTON_EVENT:
                            //TECLADO
                            switch (received_data.data.button_event)
                            {
                            case EVENT_BUTTON_1:
                                teclado_num = 1;
                                break;
                            
                            case EVENT_BUTTON_2:
                                teclado_num = 2;    
                                break;

                            case EVENT_BUTTON_3:
                                teclado_num = 3;    
                                break;

                            case EVENT_BUTTON_4:
                                teclado_num = 4;    
                                break;

                            case EVENT_NO_KEY:
                                teclado_num = 8;
                                break;
                            default:
                                teclado_num = 8;
                                break;
                            }   

                            if (teclado_num != teclado_num_prev) {
                                lcdDrawString(&dev, Cons32fx, 192, 190, (uint8_t *)string_teclado, WHITE);
                                sprintf(string_teclado, "%d", teclado_num);
                                lcdDrawString(&dev, Cons32fx, 192, 190, (uint8_t *)string_teclado, BLUE);
                                teclado_num_prev = teclado_num;
                            }
                        break;
                    
                        default:
                        break;

                        //--------Fin actualizar datos Pantalla TESTS--------//
                    
                    } //Fin switch origen

                } // Fin if contiene datos

                break; //Fin case TESTS

            case INICIAL:
                /* code */
                if (pantalla_actual != INICIAL) {
                    pantalla_actual = INICIAL;
                //--------Pantalla Inicial--------//
                //ESTAS VARIABLES SON DE PRUEBA, LUEGO VAN A VENIR DE LA COLA
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
                //--------Fin Pantalla Inicial--------//
                }
                break;  

            case BALANZA:
                /* code */
                if (pantalla_actual != BALANZA) {
                    pantalla_actual = BALANZA;
                    //Dibujar campos pantalla BALANZA//
                    //--------Campos Pantalla Balanza--------//
                        lcdDrawFillRect(&dev, 45, 0, 239, 319, WHITE);

                        //Barra superior con texto y botón 
                        lcdDrawFillRect(&dev, 0, 0, 44, 320, NARANJITA);
                        lcdDrawString(&dev, ilgh24fx, 34, 306, (uint8_t *)"Para pesar,pulse", BLACK);
                        lcdDrawFillRect(&dev, 10, 80, 34, 105, AZUL_OCEANO);
                        lcdDrawRect(&dev, 9, 81, 35, 104, BLACK);
                        lcdDrawString(&dev, ilgh24fx, 33, 97, (uint8_t *)"1", WHITE);
                        
                        //Peso actual
                        lcdDrawBMP(&dev, "/data/pesa.bmp", 32, 60);
                        lcdDrawString(&dev, Cons32fx, 85, 210, (uint8_t *)"Peso actual:", BLACK);
                        lcdDrawString(&dev, Cons32fx, 118, 180, (uint8_t *)"73,4 kg", AZUL_OCEANO);

                        //Último peso
                        lcdDrawBMP(&dev, "/data/historial.bmp", 40, 140);
                        lcdDrawString(&dev, ilgh24fx, 160, 200, (uint8_t *)"Ultimo peso:", GRIS);
                        lcdDrawString(&dev, ilgh24fx, 185, 180, (uint8_t *)"72,8 kg", GRIS);



                        //Barra inferior con opciones
                        lcdDrawFillRect(&dev, 196, 0, 239, 319, CELESTITO);

                        lcdDrawFillRect(&dev, 207, 286, 229, 309, AZUL_OCEANO);
                        lcdDrawRect(&dev, 230, 310, 206, 285, BLACK);
                        lcdDrawString(&dev, ilgh24fx, 230, 303, (uint8_t *)"2", WHITE);
                        lcdDrawString(&dev, ilgh24fx, 230, 278, (uint8_t *)"Guardar", BLACK);

                        lcdDrawFillRect(&dev, 207, 155, 229, 180, AZUL_OCEANO);
                        lcdDrawRect(&dev, 206, 156, 230, 179, BLACK);
                        lcdDrawString(&dev, ilgh24fx, 230, 172, (uint8_t *)"3", WHITE);
                        lcdDrawString(&dev, ilgh24fx, 230, 149, (uint8_t *)"Cero", BLACK);

                        lcdDrawFillRect(&dev, 207, 155, 229, 180, AZUL_OCEANO);
                        lcdDrawRect(&dev, 206, 156, 230, 179, BLACK);
                        lcdDrawString(&dev, ilgh24fx, 230, 172, (uint8_t *)"3", WHITE);
                        lcdDrawString(&dev, ilgh24fx, 230, 149, (uint8_t *)"Cero", BLACK);


                }
                break;
                

            default:

                break;
            }
        
        } //cierro xQueueReceive   
        
         vTaskDelay(pdMS_TO_TICKS(50));
         contador_vueltas++;
            if (contador_vueltas >= 2) { //Cada x vueltas ( ? segundos o más, aprox)
                flag_refresh_display_data = true;
                contador_vueltas = 0;
            }

    } //cierro while (1)

}


void simulation_task(void *pvParameters) {
	
    display_t sent_data;
    sent_data.data.origen = TEST_TASK;
    sent_data.pantalla = TESTS;

    sent_data.data.peso_total = 55.5;
    uint16_t teclado_num = 1;
    uint16_t altura = 70;
    sent_data.data.hall_on_off = true;
    sent_data.data.freno_on_off = false;

	while(1)
	{
       
    
        for (int8_t i = 0; i<76; i++) {

            //INCLINACIÓN
            sent_data.data.inclinacion = (float)i;

            //BALANZA
            if (i%4) {
                sent_data.data.peso_total += 0.5;
            }

            //BARANDALES
            if (i%12 == 0){
                sent_data.data.hall_on_off = !sent_data.data.hall_on_off;
               
            }

            //FRENO
            sent_data.data.freno_on_off = (i>45 && i<60) ? true : false;

            //TECLADO
            if ((i+1) % 20 == 0)
                teclado_num++;

            switch (teclado_num)
            {
            case 1:
                sent_data.data.button_event = EVENT_BUTTON_1;
                break;

            case 2:
            sent_data.data.button_event = EVENT_BUTTON_2;
            break;

            case 3:
            sent_data.data.button_event = EVENT_BUTTON_3;
            break;

            case 4:
            sent_data.data.button_event = EVENT_BUTTON_4;
            break;
        
            default:
            sent_data.data.button_event = EVENT_BUTTON_1;

                break;
            }

            //ALTURA
            if( i%4 == 0){
                altura = altura + 1;
            }
            sent_data.data.altura = (long)altura;
            
            //Enviar
            if (xQueueSend(display_queue, &sent_data, (TickType_t)0) != pdPASS) {
                printf( "No se pudo enviar datos a display.");
            } 
            /*else {
                    ESP_LOGI("SIMU", "Enviado: Altura: %ld, Peso: %.2f, HALL_OnOff: %d, Freno: %d, Inclinacion: %f, Boton: %d\n",
                    sent_data.data.altura,
                    sent_data.data.peso,
                    sent_data.data.hall_on_off,
                    sent_data.data.freno_on_off,
                    sent_data.data.inclinacion,
                    sent_data.data.button_event);
            }*/

            vTaskDelay(pdMS_TO_TICKS(250)); //Aumentar velocidad sacando el LOGI

        }
        sent_data.data.peso_total = 55.5;
        teclado_num = 1;
        altura = 70;



	}

}



//-----------BAKCUPS-----------//

//Backup de función auxiliar para dibujo rápido con DMA


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


/* //Prueba de dibujo de texto ángulo en pantalla inicial
    //Mover variables
    lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)"37", WHITE);
    sprintf(string_angulo, "%d", angulo);   

    
    for(angulo=37; angulo<76; angulo++) {
        lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)&string_angulo, WHITE);  
        sprintf(string_angulo, "%d", angulo);   
        lcdDrawString(&dev, Cons32fx, 105, 165, (uint8_t *)&string_angulo, AZUL_OCEANO);    
        vTaskDelay(pdMS_TO_TICKS(200));  

    }*/