# FIXME: How best to handle release build, debug build, coverage builds?
# Have separate build directories for each version; different CFLAGS for each
#
# make coverage   into build-coverage
# make debug      into build-debug
# make            into build

# -DSSM_DEBUG enables whitebox testing of the scheduler
TEST_CFLAGS = -g -DSSM_DEBUG

# --coverage enables the use of gcov
# -DNDEBUG disables testing assert coverage, which confuses the coverage tool
#COVERAGE_CFLAGS = -DSSM_DEBUG --coverage -DNDEBUG
# FIXME: use our own assert for unit testing that decays into a call of
# of a vacuous function when we're doing coverage testing.

ROOTDIR = $(realpath .)
PLATFLAGS = -O -I$(ROOTDIR)/include
ifdef PLATFORM
PLATFLAGS += -I$(ROOTDIR)/platform/$(PLATFORM)/include
ifdef BOARD
PLATFLAGS += -I$(ROOTDIR)/platform/$(PLATFORM)/boards/$(BOARD)/include \
	     -DPLATFORM_BOARD -DPLATFORM_BOARD_$(BOARD)
endif
endif
CFLAGS =  $(PLATFLAGS) -Wall -pedantic -std=c99 $(TEST_CFLAGS) $(COVERAGE_CFLAGS)

SOURCES = $(wildcard src/*.c)
INCLUDES = $(wildcard include/*.h)
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))

EXAMPLES = $(wildcard examples/*.c)
EXAMPLEEXES = $(patsubst examples/%.c, build/%, $(EXAMPLES))

RED = \e[31m
GREEN = \e[32m
RESET_COLOR = \e[0m

ARFLAGS = -cr

ifndef PLATFORM
all : test-examples test_main test-throw

test_main : build/test_main
	./build/test_main > build/test_main.out || echo "${RED}TEST_MAIN FAILED${RESET_COLOR}"
	@(diff test/test_main.out build/test_main.out && \
	echo "${GREEN}TEST_MAIN PASSED${RESET_COLOR}") || \
	echo "${RED}TEST_MAIN OUTPUT DIFFERS${RESET_COLOR}"
test-examples : examples
	./runexamples > build/examples.out
	@(diff test/examples.out build/examples.out && \
	echo "${GREEN}EXAMPLES PASSED${RESET_COLOR}") || \
	echo "${RED}EXAMPLE OUTPUT DIFFERS${RESET_COLOR}"
test-throw : build/test-throw build/test-throw-override
	timeout -v 0.01 ./build/test-throw >build/test-throw.out 2>&1 || true
	timeout -v 0.01 ./build/test-throw-override >>build/test-throw.out 2>&1 || true
	@(diff test/test-throw.out build/test-throw.out && \
	echo "${GREEN}EXAMPLES PASSED${RESET_COLOR}") || \
	echo "${RED}EXAMPLE OUTPUT DIFFERS${RESET_COLOR}"

build/test_main : test/test_main.c build/libssm.a
	$(CC) $(CFLAGS) -o $@ test/test_main.c -Lbuild -lssm
build/test-throw : test/test-throw.c build/libssm.a
	$(CC) $(CFLAGS) -o $@ test/test-throw.c -Lbuild -lssm
build/test-throw-override : test/test-throw.c test/override-throw.c build/libssm.a
	$(CC) $(CFLAGS) -o $@ test/test-throw.c test/override-throw.c -Lbuild -lssm
else # PLATFORM

ifeq ("$(wildcard platform/$(PLATFORM))","")
$(error PLATFORM must be one of the following: $(shell ls platform))
endif
ifndef BOARD
ifneq ("$(wildcard platform/$(PLATFORM)/boards/"),"")
$(error BOARD must be specified)
endif
LIBPLATFORM = build/libssm_platform.$(PLATFORM).a
else # BOARD
ifeq ("$(wildcard platform/$(PLATFORM)/boards/$(BOARD))","")
$(error BOARD must be one of the following: $(shell ls platform/$(PLATFORM)/boards))
endif
LIBPLATFORM = build/libssm_platform.$(PLATFORM).$(BOARD).a
endif # BOARD

all : build/libssm.a $(LIBPLATFORM)
	# Checking that platform build system linked in src/platform/ code
	nm $(LIBPLATFORM) | grep ssm_tick_loop > /dev/null

include platform/$(PLATFORM)/Makefile
endif # PLATFORM

# Requires COVERAGE_CFLAGS to be set
ssm-scheduler.c.gcov : build/test_main
	./build/test_main
	gcov \
	--branch-probabilities \
	--all-blocks \
	--object-directory build ssm-scheduler.c

# With --coverage given, run
# build/test_main
# mv build/*.gcda build/*.gcno .
# gcov test_main
# more test_main.c.gcov
# gcov ssm-scheduler.c

build/libssm.a : $(INCLUDES) $(OBJECTS)
	rm -f build/libssm.a
	$(AR) $(ARFLAGS) build/libssm.a $(OBJECTS)

build/%.o : src/%.c $(INCLUDES)
	$(CC) $(CFLAGS) -c -o $@ $<

examples : $(EXAMPLEEXES)

build/% : examples/%.c build/libssm.a
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lssm

documentation : doc/html/index.html

doc/html/index.html : doc/Doxyfile $(SOURCES) $(INCLUDES)
	cd doc && doxygen


.PHONY : clean
clean :
	rm -rf *.gch build/* libssm.a *.gcda *.gcno *.gcov
