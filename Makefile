LIB_NAME := ssm

BUILD_DIR := build
SRC_DIR := src
INC_DIR := include
TEST_DIR := test
EXE_DIR := examples
DOC_DIR := doc

LIB_SRC := $(wildcard $(SRC_DIR)/*.c)
LIB_INC := $(wildcard include/*.h)

LIB_OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LIB_SRC))
LIB_TGT := $(BUILD_DIR)/lib$(LIB_NAME).a

DOC_CFG := Doxyfile
DOC_SRC := README.md $(wildcard doc/*.md doc/*.dox) $(LIB_SRC) $(LIB_INC)
DOC_TGT := $(BUILD_DIR)/doc

EXE_SRC := $(wildcard $(EXE_DIR)/*.c)
EXE_OBJ := $(patsubst $(EXE_DIR)/%.c, $(BUILD_DIR)/%.o, $(EXE_SRC))
EXE_TGT := $(patsubst %.o, %, $(EXE_OBJ))

TLIB_NAME := t$(LIB_NAME)
TLIB_OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(LIB_SRC))
TLIB_TGT := $(BUILD_DIR)/lib$(TLIB_NAME).a

TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(TEST_SRC))
TEST_TGT := $(patsubst %.o, %, $(TEST_OBJ))

CC = cc
CFLAGS = -I$(INC_DIR) -O -Wall -pedantic -std=c99
TEST_CFLAGS = -g -DSSM_DEBUG

LD = cc
LDFLAGS = -L$(BUILD_DIR)
LDLIBS = -l$(LIB_NAME)

AR = ar
ARFLAGS = -cr

lib: $(LIB_TGT)
docs: $(DOC_TGT)
exes: $(EXE_TGT)
tests: $(TEST_TGT)

test-tests: $(TEST_TGT)

test-exes: $(EXE_TGT)

$(LIB_TGT): $(LIB_OBJ)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $+

$(TLIB_TGT): $(TLIB_OBJ)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $+

$(EXE_TGT): %: %.o $(LIB_TGT)
	$(LD) $(LDFLAGS) -o $@ $@.o -l$(LIB_NAME)

$(TEST_TGT): %: %.o $(TLIB_TGT)
	$(LD) $(LDFLAGS) -o $@ $@.o -l$(TLIB_NAME)

vpath %.c $(SRC_DIR) $(EXE_DIR) $(TEST_DIR)

$(LIB_OBJ) $(EXE_OBJ): $(BUILD_DIR)/%.o: %.c $(LIB_INC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TLIB_OBJ) $(TEST_OBJ): $(BUILD_DIR)/test_%.o: %.c $(LIB_INC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c -o $@ $<

# FIXME: How best to handle release build, debug build, coverage builds?
# Have separate build directories for each version; different CFLAGS for each
#
# make coverage   into build-coverage
# make debug      into build-debug
# make            into build

## -DSSM_DEBUG enables whitebox testing of the scheduler
#TEST_CFLAGS = -g -DSSM_DEBUG

## --coverage enables the use of gcov
## -DNDEBUG disables testing assert coverage, which confuses the coverage tool
##COVERAGE_CFLAGS = -DSSM_DEBUG --coverage -DNDEBUG
## FIXME: use our own assert for unit testing that decays into a call of
## of a vacuous function when we're doing coverage testing.

#SOURCES = $(wildcard src/*.c)
#INCLUDES = $(wildcard include/*.h)
#OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))
#DOCS = $(wildcard doc/*.dox) $(wildcard doc/*.md)

#EXAMPLES = $(wildcard examples/*.c)
#EXAMPLEEXES = $(patsubst examples/%.c, build/%, $(EXAMPLES))

#lib: $(BUILDDIR)/$(LIB)

#all : test-examples test_main

#test_main : build/test_main
#	./build/test_main > build/test_main.out || echo "TEST_MAIN FAILED"
#	@(diff test/test_main.out build/test_main.out && \
#	echo "TEST_MAIN PASSED") || \
#	echo "TEST_MAIN OUTPUT DIFFERS"
#test-examples : examples
#	./runexamples > build/examples.out
#	@(diff test/examples.out build/examples.out && \
#	echo "EXAMPLES PASSED") || \
#	echo "EXAMPLE OUTPUT DIFFERS"

#build/test_main : test/test_main.c build/libssm.a
#	$(CC) $(CFLAGS) -o $@ test/test_main.c -Lbuild -lssm

## Requires COVERAGE_CFLAGS to be set
#ssm-scheduler.c.gcov : build/test_main
#	./build/test_main
#	gcov \
#	--branch-probabilities \
#	--all-blocks \
#	--object-directory build ssm-scheduler.c

## With --coverage given, run
## build/test_main
## mv build/*.gcda build/*.gcno .
## gcov test_main
## more test_main.c.gcov
## gcov ssm-scheduler.c

#build/%.o : src/%.c $(INCLUDES)
#	$(CC) $(CFLAGS) -c -o $@ $<

#examples : $(EXAMPLEEXES)

#build/% : examples/%.c build/libssm.a
#	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lssm

$(DOC_TGT): $(DOC_CFG) $(DOC_SRC) | $(BUILD_DIR)
	doxygen

$(BUILD_DIR):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf build
	# *.gch build/* libssm.a *.gcda *.gcno *.gcov
