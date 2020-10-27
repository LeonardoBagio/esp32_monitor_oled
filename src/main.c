#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "driver/gpio.h"
#include <u8g2.h>
#include "u8g2_esp32_hal.h"
#include "sdkconfig.h"
#include <dht.h>
#include <ultrasonic.h>
#include <driver/spi_master.h>
#include <stdio.h>
#include <string.h>

#define TRIG            GPIO_NUM_0
#define ECHO            GPIO_NUM_2
#define PIN_SDA         GPIO_NUM_5
#define PIN_SCL         GPIO_NUM_4
#define DHT_GPIO        GPIO_NUM_14
#define BUTTON          GPIO_NUM_16
#define EIXO_X          10

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = DHT_GPIO;

u8g2_t u8g2;
QueueHandle_t bufferTemperatura; 
QueueHandle_t bufferTemperaturaMaxima; 
QueueHandle_t bufferTemperaturaMinima;
QueueHandle_t bufferUmidade;
QueueHandle_t bufferUmidadeMaxima;
QueueHandle_t bufferUmidadeMinima;

void app_main(void);
void inicializarDisplay(void);
void inicializarGpio(void);
void imprimirTemperatura(void);
void imprimirUmidade(void);
void imprimirDistancia(void);
void imprimirGrafico(int eixoX, int eixoY, int valor, char porcentagem[10]);
void task_dht(void *pvParamters);
void task_oLED(void *pvParameters);

void task_dht(void *pvParamters){
    int16_t temperatura;
    int16_t umidade;
    int temperaturaMaxima   = 0;
    int umidadeMaxima       = 0;
    int temperaturaMinima   = 100;
    int umidadeMinima       = 100;

    gpio_set_pull_mode( dht_gpio , GPIO_PULLUP_ONLY);
    
    while(1){
        dht_read_data(sensor_type, dht_gpio, &umidade, &temperatura);
        umidade     = umidade / 10;
        temperatura = temperatura / 10;

        xQueueReceive(bufferTemperaturaMaxima,&temperaturaMaxima,pdMS_TO_TICKS(0));
        xQueueReceive(bufferTemperaturaMinima,&temperaturaMinima,pdMS_TO_TICKS(0));
        xQueueReceive(bufferUmidadeMaxima,&umidadeMaxima,pdMS_TO_TICKS(0));
        xQueueReceive(bufferUmidadeMinima,&umidadeMinima,pdMS_TO_TICKS(0));

        if (temperaturaMaxima <= temperatura){
            temperaturaMaxima = temperatura;
        }

        if (temperaturaMinima >= temperatura){
            temperaturaMinima = temperatura;
        }

        if (umidadeMaxima <= umidade){
            umidadeMaxima = umidade;
        }

        if (umidadeMinima >= umidade){
            umidadeMinima = umidade;
        }        

        xQueueSend(bufferTemperatura,&temperatura,pdMS_TO_TICKS(0));
        xQueueSend(bufferTemperaturaMaxima,&temperaturaMaxima,pdMS_TO_TICKS(0));
        xQueueSend(bufferTemperaturaMinima,&temperaturaMinima,pdMS_TO_TICKS(0));
        xQueueSend(bufferUmidade,&umidade,pdMS_TO_TICKS(0));
        xQueueSend(bufferUmidadeMaxima,&umidadeMaxima,pdMS_TO_TICKS(0));
        xQueueSend(bufferUmidadeMinima,&umidadeMinima,pdMS_TO_TICKS(0));
        
        vTaskDelay(1500/portTICK_PERIOD_MS);
    }
}

void inicializarDisplay(){
	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = PIN_SDA;
	u8g2_esp32_hal.scl = PIN_SCL;
	u8g2_esp32_hal_init(u8g2_esp32_hal);

	u8g2_Setup_ssd1306_i2c_128x64_noname_f(
		&u8g2,
		U8G2_R0,
		u8g2_esp32_i2c_byte_cb,
		u8g2_esp32_gpio_and_delay_cb);
	
	u8x8_SetI2CAddress(&u8g2.u8x8, 0x78); 
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2,u8g2_font_6x10_mf);
    u8g2_SendBuffer(&u8g2);
}

void inicializarGpio(){
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);
}

void task_oLED(void *pvParameters){
    int tela = 1;

    inicializarDisplay();
    inicializarGpio();

    while(1) { 
        if (!gpio_get_level(BUTTON)){
            tela = tela + 1;

            if (tela > 3){
                tela = 1;
            }
        }

        if (tela == 1){
            imprimirTemperatura();
        } else if (tela == 2){
            imprimirUmidade();
        } else {
            imprimirDistancia();
        }
    }
}

void imprimirTemperatura(){
    uint16_t temperatura;
    uint16_t maxima;
    uint16_t minima;
    char stringTemperatura[10];
    char stringMaxima[10];
    char stringMinima[10];

    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_DrawUTF8(&u8g2, EIXO_X, 15, "Temperatura");
    
    xQueueReceive(bufferTemperatura,&temperatura,pdMS_TO_TICKS(2000));
    xQueueReceive(bufferTemperaturaMaxima,&maxima,pdMS_TO_TICKS(2000));
    xQueueReceive(bufferTemperaturaMinima,&minima,pdMS_TO_TICKS(2000));    
    
    sprintf(stringTemperatura, "%d", temperatura);
    sprintf(stringMaxima, "%d", maxima);
    sprintf(stringMinima, "%d", minima);

    u8g2_DrawUTF8(&u8g2, EIXO_X, 60, "Max:");
    u8g2_DrawUTF8(&u8g2, EIXO_X + 25, 60, stringMaxima);
    u8g2_DrawUTF8(&u8g2, 75, 60, "Min:");
    u8g2_DrawUTF8(&u8g2, 100, 60, stringMinima);

    imprimirGrafico(EIXO_X, 25, temperatura, stringTemperatura);
}

void imprimirUmidade(){
    uint16_t umidade;
    uint16_t maxima;
    uint16_t minima;
    char stringUmidade[10];
    char stringMaxima[10];
    char stringMinima[10];

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_DrawUTF8(&u8g2, EIXO_X, 15, "Umidade");
    
    xQueueReceive(bufferUmidade,&umidade,pdMS_TO_TICKS(2000));
    xQueueReceive(bufferUmidadeMaxima,&maxima,pdMS_TO_TICKS(2000));
    xQueueReceive(bufferUmidadeMinima,&minima,pdMS_TO_TICKS(2000));    
    
    sprintf(stringUmidade, "%d", umidade);
    sprintf(stringMaxima, "%d", maxima);
    sprintf(stringMinima, "%d", minima);

    u8g2_DrawUTF8(&u8g2, EIXO_X, 60, "Max:");
    u8g2_DrawUTF8(&u8g2, EIXO_X + 25, 60, stringMaxima);
    u8g2_DrawUTF8(&u8g2, 75, 60, "Min:");
    u8g2_DrawUTF8(&u8g2, 100, 60, stringMinima);

    imprimirGrafico(EIXO_X, 25, umidade, stringUmidade);    
}

void imprimirDistancia(){
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_DrawUTF8(&u8g2, 15, 15, "Distancia");
    imprimirGrafico(15, 35, 100, "100");
}

void imprimirGrafico(int eixoX, int eixoY, int valor, char porcentagem[10]){
    if (valor > 90){
        valor = 90;
    }

    u8g2_DrawBox(&u8g2, eixoX, eixoY, valor, 7);
    u8g2_DrawUTF8(&u8g2, eixoX + valor + 3, eixoY + 7, porcentagem);
    u8g2_SendBuffer(&u8g2);
    u8g2_DrawBox(&u8g2, 0, 0, 0, 0);
    u8g2_ClearBuffer(&u8g2);    
}

void app_main() {
    bufferTemperatura       = xQueueCreate(5,sizeof(uint16_t)); 
    bufferTemperaturaMaxima = xQueueCreate(5,sizeof(uint16_t)); 
    bufferTemperaturaMinima = xQueueCreate(5,sizeof(uint16_t));
    bufferUmidade           = xQueueCreate(5,sizeof(uint16_t));
    bufferUmidadeMaxima     = xQueueCreate(5,sizeof(uint16_t)); 
    bufferUmidadeMinima     = xQueueCreate(5,sizeof(uint16_t)); 

    xTaskCreate(task_oLED,"task_oLED",2048, NULL, 1, NULL);
    xTaskCreate(task_dht,"task_dht",2048,NULL, 2, NULL);
}