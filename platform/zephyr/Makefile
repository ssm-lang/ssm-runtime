CC = arm-none-eabi-gcc

export PLATFORMIO_BUILD_FLAGS = $(PLATFLAGS)
PLATFORMIO_WORKSPACE_DIR = $(ROOTDIR)/build/platform/zephyr
export PLATFORMIO_WORKSPACE_DIR

PLAT_GEN_SRCS = $(wildcard src/platform/*.c)
PLAT_SRCS = $(wildcard platform/zephyr/src/*.c)
PLAT_OBJS = $(patsubst src/platform/%.c, \
	    $(PLATFORMIO_WORKSPACE_DIR)/build/$(BOARD)/src/platform/%.o, $(PLAT_GEN_SRCS))  \
	    $(patsubst platform/zephyr/src/%.c, \
	    $(PLATFORMIO_WORKSPACE_DIR)/build/$(BOARD)/src/%.o, $(PLAT_SRCS))

.PHONY: $(LIBPLATFORM)
$(LIBPLATFORM) :
	pio run -d platform/zephyr/ -e $(BOARD) -t src/
	$(AR) $(ARFLAGS) $@ $(PLAT_OBJS)
