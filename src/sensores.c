#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include <dht.h>
#include <ultrasonic.h>

#define DHT_GPIO GPIO_NUM_0

#define MAX_DISTANCE_CM 500
#define TRIGGER_GPIO GPIO_NUM_16
#define ECHO_GPIO GPIO_NUM_12

void app_main(void);
void task_dht(void *pvParamters);
void task_ultrasonic(void *pvParamters);

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = DHT_GPIO;

static const char *TAG = "sensores";

void task_dht(void *pvParamters)
{
	int16_t temperatura;
	int16_t umidade;
	gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
	while (1)
	{
		if (dht_read_data(sensor_type, dht_gpio, &umidade, &temperatura) == ESP_OK)
		{
			umidade = umidade / 10;
			temperatura = temperatura / 10;
			ESP_LOGI(TAG, "Umidade %d%% e Temperatura %dºc", umidade, temperatura);
		}
		else
		{
			ESP_LOGE(TAG, "Não foi possivel ler o sensor");
		}
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void task_ultrasonic(void *pvParamters)
{
	ultrasonic_sensor_t sensor = {
		.trigger_pin = TRIGGER_GPIO,
		.echo_pin = ECHO_GPIO};
	ultrasonic_init(&sensor);

	while (1)
	{
		uint32_t distance;
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
		if (res != ESP_OK)
		{
			ESP_LOGI(TAG, "Error: ");
			switch (res)
			{
			case ESP_ERR_ULTRASONIC_PING:
				ESP_LOGI(TAG, "Cannot ping (device is in invalid state)\n");
				break;
			case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
				ESP_LOGI(TAG, "Ping timeout (no device found)\n");
				break;
			case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
				ESP_LOGI(TAG, "Echo timeout (i.e. distance too big)\n");
				break;
			default:
				ESP_LOGI(TAG, "%d\n", res);
			}
		}
		else
			ESP_LOGI(TAG, "Distance: %d cm\n", distance);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "inicio...");
	xTaskCreate(task_dht, "task_dht", 2048, NULL, 1, NULL);
	xTaskCreate(task_ultrasonic, "task_ultrasonic", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}