#include <stdio.h>
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <ssm-internal.h>

gptimer_handle_t timer;
SemaphoreHandle_t sem;

static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
	BaseType_t xHighPriTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( sem, &xHighPriTaskWoken);
	return false;
}

static void platform_timer_start()
{
	timer = NULL;
	gptimer_config_t timer_config = {
    		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
    		.direction = GPTIMER_COUNT_UP,
    		.resolution_hz = 1 * 1000 * 1000 // 1 tick = 1us (microsecond)
	};
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));
	gptimer_event_callbacks_t cbs = {
					.on_alarm = timer_on_alarm_cb,
	};
	ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));
	ESP_ERROR_CHECK(gptimer_enable(timer));
	ESP_ERROR_CHECK(gptimer_start(timer));
}

typedef uint64_t ssm_time_t;

void ssm_throw(ssm_error_t reason, const char *file, int line, const char *func) {
	printf("ssm_throw is being called! \n");
	fflush(stdout);
}

static void tick_loop(void)
{
	printf("Entering tick_loop\n");
	fflush(stdout);
	platform_timer_start();
	ssm_tick();
	while (true) {
		ssm_time_t wall_time;
		esp_err_t err = gptimer_get_raw_count(timer, &wall_time);
		if (err != ESP_OK) {
			SSM_THROW(SSM_PLATFORM_ERROR);
			return;
		}
		ssm_time_t next_time = ssm_next_event_time();
		ssm_time_t scaled_next_time = next_time/1000;
		printf("current time: %llu \n", wall_time);
		printf("next time: %llu \n", scaled_next_time);
		fflush(stdout);
		
    	if (scaled_next_time <= wall_time) {
			printf("Calling ssm_tick \n");
			fflush(stdout);
      		ssm_tick();

    	} else {
			fflush(stdout);
      		if (next_time == SSM_NEVER) {
				printf("next time is NEVER \n");
				fflush(stdout);
				int num_ticks_to_wait = 1000000;
				while(xSemaphoreTake( sem, (TickType_t) num_ticks_to_wait) == pdFALSE){}
      		} else {
				
				gptimer_alarm_config_t alarm_config = {
					.alarm_count = scaled_next_time,
				};
				ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));
				printf("Set alarm for next time \n");
				fflush(stdout);
				int num_ticks_to_wait = 1000000;
				while(xSemaphoreTake( sem, (TickType_t) num_ticks_to_wait) == pdFALSE){}
      		}
    	}
	}
}

#ifndef SSM_NUM_PAGES
#define SSM_NUM_PAGES 32
#endif

static char mem[SSM_NUM_PAGES][SSM_MEM_PAGE_SIZE] = {0};
static size_t allocated_pages = 0;

static void *alloc_page(void)
{
  if (allocated_pages >= SSM_NUM_PAGES)
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
  return mem[allocated_pages++];
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }

static void init_ssm(void)
{
	printf("Entering init_ssm\n");
	fflush(stdout);

	ssm_mem_init(alloc_page, alloc_mem, free_mem);

	extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                  ssm_value_t *, ssm_value_t *);
	ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
	ssm_value_t nothing1 = ssm_new_sv(ssm_marshal(0));
	ssm_value_t main_argv[2] = {nothing0, nothing1};
	ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH, main_argv, NULL));
}



void app_main(void)
{
	printf("Entering app_main\n");
	fflush(stdout);
	sem = xSemaphoreCreateBinary();
	if (sem == NULL){
		printf("Error creating semaphore, restarting\n");
		fflush(stdout);
    	esp_restart();
	}
	init_ssm();
	tick_loop();
}
