#include <stdio.h>
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"

#define BLINK_GPIO GPIO_NUM_2

SemaphoreHandle_t xSemaphore;
int max_takes;

static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
	BaseType_t xHighPriTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( xSemaphore, &xHighPriTaskWoken);
	return false;
}

void app_main(void)
{
	printf("running\r\n");
	max_takes = 10;
	xSemaphore = xSemaphoreCreateBinary();
	if (xSemaphore == NULL){
		printf("error creating semaphore, restarting\r\n");
		fflush(stdout);
    		esp_restart();
	}
	// Must give sem once before taking
	printf("sem created \r\n");
	gptimer_handle_t gptimer = NULL;
	gptimer_config_t timer_config = {
    		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
    		.direction = GPTIMER_COUNT_UP,
    		.resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
	};
	
	gpio_reset_pin(BLINK_GPIO);
    	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);	
	gpio_set_level(BLINK_GPIO, 0);	
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
	printf("timer config set \r\n");	
	gptimer_alarm_config_t alarm_config = {
    		.alarm_count = 1000000, // initial alarm target = 1s @resolution 1MHz
	};
	ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
	printf("alarm set\r\n");
	gptimer_event_callbacks_t cbs = {
		.on_alarm = timer_on_alarm_cb,
	};
	ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
	printf("callback registered \r\n");
	ESP_ERROR_CHECK(gptimer_enable(gptimer));
	printf("timer enabled\r\n");
	ESP_ERROR_CHECK(gptimer_start(gptimer));
	printf("timer started \r\n");
	fflush(stdout);
	int curr_level = 0;
	int num_ticks_to_wait = 10;
	int original = max_takes;
	while(true){
		if(max_takes <= 0){
			break;
		}
		// No need to give sem back since isr gives
		if(xSemaphoreTake( xSemaphore, (TickType_t) num_ticks_to_wait) == pdTRUE){
			printf("Took sem \r\n");
			if (curr_level == 0){
				gpio_set_level(BLINK_GPIO, 1);
			}else {
				gpio_set_level(BLINK_GPIO, 0);
			}
			curr_level = (curr_level == 0) ? 1 : 0;	
			max_takes--;
			uint64_t raw_count;
			esp_err_t err = gptimer_get_raw_count(gptimer, &raw_count);
			if (err != ESP_OK){
				printf("err during timer raw count read \r\n");
				break;
			}
			gptimer_alarm_config_t alarm_config = {
        			.alarm_count = raw_count + 1000000, // alarm in next 1s
    			};
			gptimer_set_alarm_action(gptimer, &alarm_config);
		} else{
			printf("Unable to take sem within %d ticks, looping again \r\n", num_ticks_to_wait);
		}
		fflush(stdout);
	}
	printf("Took sem %d times, restarting \r\n", original - max_takes);
    	fflush(stdout);
    	esp_restart();
}
