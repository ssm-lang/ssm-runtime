#ifndef _BOARD_SPECIFIC_SSM_IO_H
#define _BOARD_SPECIFIC_SSM_IO_H

#include <toolchain/common.h>

#include <platform-specific/ssm-io.h>

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay),
             "sw0 device alias not defined");

#define SSM_IN_SPEC_0 GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios)
#define SSM_IN_INIT_0 initialize_input_gpio_event

#define SSM_IN_COUNT 1

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay),
             "led0 device alias not defined");
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(led1), okay),
             "led1 device alias not defined");
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(led2), okay),
             "led2 device alias not defined");
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_ALIAS(led3), okay),
             "led3 device alias not defined");

#define SSM_OUT_SPEC_0 GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)
#define SSM_OUT_INIT_0 initialize_output_gpio_bool
#define SSM_OUT_SPEC_1 GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios)
#define SSM_OUT_INIT_1 initialize_output_gpio_bool
#define SSM_OUT_SPEC_2 GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios)
#define SSM_OUT_INIT_2 initialize_output_gpio_bool
#define SSM_OUT_SPEC_3 GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios)
#define SSM_OUT_INIT_3 initialize_output_gpio_bool

#define SSM_OUT_COUNT 4

#endif /* _BOARD_SPECIFIC_SSM_IO_H */
