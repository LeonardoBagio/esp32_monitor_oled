#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <u8g2.h>
#include "u8g2_esp32_hal.h"

#define DHT_GPIO        GPIO_NUM_0
#define PIN_SDA         GPIO_NUM_5
#define PIN_SCL         GPIO_NUM_4

static const char *TAG = "EX10";
//static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = DHT_GPIO;
u8g2_t u8g2;
QueueHandle_t bufferTemperatura; 

void app_main (void);
void task_dht(void *pvParamters);
void task_oLED(void *pvParameters);

void task_dht(void *pvParamters)
{
    int16_t temperatura;
    int16_t umidade;
    gpio_set_pull_mode( dht_gpio , GPIO_PULLUP_ONLY);
    while(1)
    {
        umidade     = esp_random()/100000000;
        temperatura = esp_random()/100000000;

        xQueueSend(bufferTemperatura,&temperatura,pdMS_TO_TICKS(0));

        ESP_LOGI(TAG,"Umidade %d%% e Temperatura %dºC", umidade, temperatura );

        /*if(dht_read_data(sensor_type, dht_gpio, &umidade, &temperatura) == ESP_OK)
        { 
            umidade = umidade / 10; //Quem tem sensor
            temperatura = temperatura / 10; //Quem tem sensor
            xQueueSend(bufferTemperatura,&temperatura,pdMS_TO_TICKS(0)); 
            //umidade = esp_random()/1000000;
            //temperatura = esp_random()/1000000;
            ESP_LOGI(TAG,"Umidade %d%% e Temperatura %dºC",umidade, temperatura );
        }
        else
        {
            ESP_LOGE(TAG, "Não foi possível ler o sensor"); 
        } */       
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void task_oLED(void *pvParameters)
{
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
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_SetFont(&u8g2,u8g2_font_6x10_mf);
    u8g2_DrawUTF8(&u8g2,15,15,"IoT Aplicada");
    u8g2_SendBuffer(&u8g2);
    uint16_t temp;
    char stringTemperatura[10];
    while(1)
    {
        xQueueReceive(bufferTemperatura,&temp,pdMS_TO_TICKS(2000));
        u8g2_DrawUTF8(&u8g2,15,30,"Umidade (%): ");
        sprintf(stringTemperatura,"%d",temp);
        u8g2_DrawUTF8(&u8g2,80,30,stringTemperatura);
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(100/portTICK_PERIOD_MS);
        u8g2_DrawBox(&u8g2, 0, 0, 5, 100);
    }
}

void app_main() {
    bufferTemperatura = xQueueCreate(5,sizeof(uint16_t)); 
    xTaskCreate(task_dht,"task_dht",2048,NULL,1,NULL);
    xTaskCreate(task_oLED,"task_oLED",2048, NULL, 2, NULL);
}