#include <stdio.h>

#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"

#include <ssm-platform.h>
#include <ssm-internal.h>


typedef uint64_t ssm_time_t;

#define BLINK_GPIO GPIO_NUM_2

gptimer_handle_t timer;
SemaphoreHandle_t tick_sem;


static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
	BaseType_t xHighPriTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( tick_sem, &xHighPriTaskWoken);
	return false;
}

void ssm_platform_timer_start()
{
	timer = NULL;
	gptimer_config_t timer_config = {
    		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
    		.direction = GPTIMER_COUNT_UP,
    		.resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
	};
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));
	gptimer_alarm_config_t alarm_config = {
    		.alarm_count = 1000000, // initial alarm target = 1s @resolution 1MHz
	};
	ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));
	gptimer_event_callbacks_t cb = {
		.on_alarm = timer_on_alarm_cb,
	};
	ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cb, NULL));
	ESP_ERROR_CHECK(gptimer_enable(timer));
	ESP_ERROR_CHECK(gptimer_start(timer));
}

void tick_loop(void)
{
	ssm_platform_timer_start();
	ssm_tick();// TODO
	for (;;) {
		ssm_time_t platform_time;
		esp_err_t err = gptimer_get_raw_count(gptimer, &platform_time);
		if (err != ESP_OK){
			printf("err during timer raw count read \n");
			fflush(stdout);
			break;
		}
		ssm_time_t next_event_time = ssm_next_event_time();// TODO
		// Check if there is an input somehow? For now assume none now but one exists in the future
		int has_input = 0;
		int has_future_input = 1;
		if (has_input) {
			// Schedule event
		} else if (platform_time <= next_event_time) {
			ssm_tick();
		} else if (has_future_input){
			//TODO: Move to the top of the loop
			int num_ticks_to_wait = 10; // TODO: Remove and set INCLUDE_vTaskSuspend in config fail to wait indef
			xSemaphoreTake( xSemaphore, (TickType_t) num_ticks_to_wait);
		}
	}	
}

void app_main(void)
{
	tick_sem = xSemaphoreCreateBinary();
	if (tick_sem == NULL){
		printf("error creating semaphore, restarting\n");
		fflush(stdout);
    		esp_restart();
	}
	gpio_reset_pin(BLINK_GPIO);
    	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);	
	gpio_set_level(BLINK_GPIO, 0);
	tick_loop();
}
