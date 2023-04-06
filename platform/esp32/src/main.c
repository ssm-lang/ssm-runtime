#include <stdio.h>
#include <stdatomic.h>

#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"

#include <ssm-platform.h>
#include <ssm-internal.h>


gptimer_handle_t timer;
SemaphoreHandle_t tick_sem;


static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
	BaseType_t xHighPriTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( tick_sem, &xHighPriTaskWoken);
	return false;
}

// Figure out the gpio code from "blink" in ssl
// i want just write a ssl program and hit run and it should run

static void ssm_platform_timer_start()
{
	timer = NULL;
	gptimer_config_t timer_config = {
    		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
    		.direction = GPTIMER_COUNT_UP,
    		.resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us  TODO: library assumes nanosecond res
	};
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));
	gptimer_event_callbacks_t cbs = {
					.on_alarm = timer_on_alarm_cb,
	};
	ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));
	ESP_ERROR_CHECK(gptimer_enable(timer));
	ESP_ERROR_CHECK(gptimer_start(timer));
	//set callback here
}


typedef uint64_t ssm_time_t;
typedef uint64_t ssm_raw_time_t;
typedef union {
  	ssm_raw_time_t raw;
  	ssm_time_t ssm;
} ssm_platform_time_t;

struct ssm_input {
  ssm_sv_t *sv;
  ssm_value_t payload;
  ssm_platform_time_t time;
};

#ifndef SSM_INPUT_RB_SIZE
#define SSM_INPUT_RB_SIZE 32
#endif

struct ssm_input ssm_input_rb[SSM_INPUT_RB_SIZE];

#define ssm_input_idx(i) ((i) % SSM_INPUT_RB_SIZE)
#define ssm_input_get(i) (&ssm_input_rb[ssm_input_idx(i)])

#define ssm_input_read_ready(r, w) (ssm_input_idx(r) != ssm_input_idx(w))
#define ssm_input_write_ready(r, w) (ssm_input_read_ready(r, w + 1))

atomic_int rb_r;
atomic_int rb_w;

static void poll_input_queue(size_t *r, size_t *w) {
	*r = atomic_load(&rb_r);
	*w = atomic_load(&rb_w);
	// TODO: Figure out what is the purpose of this?
	for (size_t scaled = 0; scaled < *w; scaled++) {
		ssm_input_get(scaled)->time.ssm = ssm_input_get(scaled)->time.raw;
	}
}

static size_t consume_input_queue(size_t r, size_t w) {
	if (!ssm_input_read_ready(r, w))
		return r;
  	
	ssm_time_t packet_time = ssm_input_get(r)->time.ssm;

	if (ssm_next_event_time() < packet_time)
		return r;
	do {
		ssm_sv_later_unsafe(ssm_input_get(r)->sv, packet_time, ssm_input_get(r)->payload);
	} while (ssm_input_read_ready(++r, w) && packet_time == ssm_input_get(r)->time.ssm);
  	
	return r;
}

static void tick_loop(void)
{
	ssm_platform_timer_start();
	//TODO: setup entry point -> setup ssm scheduler?
	ssm_tick();
	while (true) {
		ssm_time_t wall_time;
		esp_err_t err = gptimer_get_raw_count(timer, &wall_time);
		if (err != ESP_OK) {
			SSM_THROW(SSM_PLATFORM_ERROR);
			return;
		}
		ssm_time_t next_time = ssm_next_event_time();
		//TODO: Need barrier?

		size_t r, w;

    	poll_input_queue(&r, &w);

		if (ssm_input_read_ready(r, w)) {
			if (ssm_input_get(r)->time.ssm <= next_time) {
				r = consume_input_queue(r, w);
				atomic_store(&rb_r, r);
				goto do_tick;
			}
		}

    	if (next_time <= wall_time) {
		do_tick:
      		ssm_tick();

    	} else {

      		if (next_time == SSM_NEVER) {
        		//TODO: Edit config file to create to wait indef
				int num_ticks_to_wait = 1000000;

				while(xSemaphoreTake( tick_sem, (TickType_t) num_ticks_to_wait) == pdFALSE){
					printf("waiting in SSM never \n");
				}
      		} else {

				gptimer_alarm_config_t alarm_config = {
					.alarm_count = next_time,
				};
				ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));
				
				//TODO: Edit config file to create to wait indef
				int num_ticks_to_wait = 1000000;

				while(xSemaphoreTake( tick_sem, (TickType_t) num_ticks_to_wait) == pdFALSE){
					printf("waiting for next event \n");
				}
				// TODO: Cancel existing alarm that could from environment

				// It's possible that the alarm went off before we cancelled it; make
				// sure that its sem_give doesn't cause premature wake-up later on.
				//TODO: Ask John about what reset is
				tick_sem = xSemaphoreCreateBinary();
				if (tick_sem == NULL){
					SSM_THROW(SSM_PLATFORM_ERROR);
					return;
				}
      		}
    	}
	}
}

#ifndef SSM_NUM_PAGES
#define SSM_NUM_PAGES 32
#endif

static char mem[SSM_NUM_PAGES][SSM_MEM_PAGE_SIZE] = {0};
static size_t allocated_pages = 0;

static void *alloc_page(void) {
  if (allocated_pages >= SSM_NUM_PAGES)
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
  return mem[allocated_pages++];
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }


void app_main(void)
{
	printf("Setting up stuff \n");
	ssm_mem_init(alloc_page, alloc_mem, free_mem);
	tick_sem = xSemaphoreCreateBinary();
	if (tick_sem == NULL){
		printf("error creating semaphore, restarting\n");
		fflush(stdout);
    	esp_restart();
	}

	extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
	ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
	ssm_value_t nothing1 = ssm_new_sv(ssm_marshal(0));
	ssm_value_t main_argv[2] = {nothing0, nothing1};

	/* Start up main routine */
	ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
								main_argv, NULL));
	printf("Entering tick loop \n");
	tick_loop();
}

void ssm_throw(ssm_error_t reason, const char *file, int line, const char *func) {

}

//TODO: ssm platform entry and ssm_insert_input

// Function that needs to be called to add inputs
int ssm_insert_input(ssm_sv_t *sv, ssm_value_t val) {
	size_t w, r;

	ssm_raw_time_t raw_time;
	esp_err_t err = gptimer_get_raw_count(timer, &raw_time);
	if (err != ESP_OK) {
		return -1;
	}

	w = atomic_load(&rb_w);
	r = atomic_load(&rb_r);

	if (ssm_input_write_ready(r, w)) {
		struct ssm_input *pkt = ssm_input_get(w);

		pkt->sv = sv;
		pkt->payload = val;
		pkt->time.raw = raw_time;

		atomic_store(&rb_w, w + 1);

		xSemaphoreGive(tick_sem);

		return 0;
	} else {
		return -1;
  }
}



//