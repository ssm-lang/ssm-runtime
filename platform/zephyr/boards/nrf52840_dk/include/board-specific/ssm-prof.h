/** Low-latency SSM GPIO profiler.
 *
 * Interfaces directly with the HAL, meant to be used for profiling. Invoke
 * using SSM_PROF(sym) to write 16-bit symbols to GPIO-pins.
 *
 * Uses GPIO _set and _clear registers to avoid needing to RMW. This causes some
 * amount of glitching between when the two writes are peformed.
 *
 * When SSM_PROF_CLK is defined, the SSM_PROF_CLK-th bit is used as a clock
 * signal to indicate when the GPIO pin values are valid, though setting this
 * doubles the amount of time needed to invoke SSM_PROF. The SSM_PROF_BIT_n
 * values are valid when SSM_PROF_CLK is set, and invalid otherwise. This design
 * allows a logic analyzer to trigger on SSM_PROF_CLK.
 *
 * When SSM_PROF_CLK is not defined, it is the job of the logic analyzer to
 * filter out the glitches present in the GPIO pin signal.
 *
 * Allows code running on NRF boards to quickly emit up to 4 bits (optionally
 * 5 bits for clock signal) of data via GPIO, for performance profiling.
 *
 * TODO: write config for enabling clock.
 * TODO: make this optional for platforms without GPIO
 */
#ifndef _BOARD_SPECIFIC_SSM_PROF_H
#define _BOARD_SPECIFIC_SSM_PROF_H

#include <drivers/gpio.h>
#include <hal/nrf_gpio.h>

#define SSM_PROF_GPIO_PORT 1
#define SSM_PROF_GPIO_PERIPHERAL NRF_P1

#define SSM_PROF_PIN_0 10
#define SSM_PROF_PIN_1 11
#define SSM_PROF_PIN_2 12
#define SSM_PROF_PIN_3 13
#define SSM_PROF_PIN_CLK 14

#ifdef SSM_PROF_PIN_CLK
#define z_gpio_init()                                                          \
  do {                                                                         \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_0)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_1)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_2)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_3)); \
    nrf_gpio_cfg_output(                                                       \
        NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_CLK));               \
  } while (0)
#else
#define z_gpio_init()                                                          \
  do {                                                                         \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_0)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_1)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_2)); \
    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(SSM_PROF_GPIO_PORT, SSM_PROF_PIN_3)); \
  } while (0)
#endif

#define z_gpio_set(val) nrf_gpio_port_out_set(SSM_PROF_GPIO_PERIPHERAL, val)
#define z_gpio_clear(val) nrf_gpio_port_out_clear(SSM_PROF_GPIO_PERIPHERAL, val)

#define SSM_PROF_INIT() z_gpio_init()

#define SSM_PROF_VAL(n) (1u << SSM_PROF_PIN_##n)

#define SSM_PROF_SYM_VAL(sym, n) ((sym) & (1u << (n)) ? SSM_PROF_VAL(n) : 0)

#define SSM_PROF_SYM(sym)                                                      \
  (SSM_PROF_SYM_VAL(sym, 0) | SSM_PROF_SYM_VAL(sym, 1) |                       \
   SSM_PROF_SYM_VAL(sym, 2) | SSM_PROF_SYM_VAL(sym, 3))

#define SSM_PROF_MASK                                                          \
  ((SSM_PROF_VAL(0)) | (SSM_PROF_VAL(1)) | (SSM_PROF_VAL(2)) |                 \
   (SSM_PROF_VAL(3)))

#ifdef SSM_PROF_PIN_CLK
#define SSM_PROF(sym)                                                          \
  do {                                                                         \
    compiler_barrier();                                                        \
    z_gpio_clear(SSM_PROF_VAL(CLK));                                           \
    compiler_barrier();                                                        \
    z_gpio_set(SSM_PROF_MASK &SSM_PROF_SYM(sym));                              \
    z_gpio_clear(SSM_PROF_MASK & ~SSM_PROF_SYM(sym));                          \
    compiler_barrier();                                                        \
    z_gpio_set(SSM_PROF_VAL(CLK));                                             \
    compiler_barrier();                                                        \
  } while (0)
#else
#define SSM_PROF(sym)                                                          \
  do {                                                                         \
    compiler_barrier();                                                        \
    z_gpio_set(SSM_PROF_MASK &SSM_PROF_SYM(sym));                              \
    z_gpio_clear(SSM_PROF_MASK & ~SSM_PROF_SYM(sym));                          \
    compiler_barrier();                                                        \
  } while (0)
#endif

#endif
