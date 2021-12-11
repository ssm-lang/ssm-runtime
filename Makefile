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
EXE_INC := $(wildcard $(EXE_DIR)/*.h)

TLIB_NAME := t$(LIB_NAME)
TLIB_OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(LIB_SRC))
TLIB_TGT := $(BUILD_DIR)/lib$(TLIB_NAME).a

TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(TEST_SRC))
TEST_TGT := $(patsubst %.o, %, $(TEST_OBJ))

COV_TGT := $(patsubst $(BUILD_DIR)/test_%.o, $(BUILD_DIR)/%.c.gcov, $(TLIB_OBJ))

CC = cc
CFLAGS = -g -I$(INC_DIR) -O -Wall -pedantic -std=c99
TEST_CFLAGS = $(CFLAGS) -g -DSSM_DEBUG --coverage

LD = cc
LDFLAGS = -L$(BUILD_DIR)
TEST_LDFLAGS = $(LDFLAGS) --coverage

AR = ar
ARFLAGS = -cr

GCOV = gcov
GCOVFLAGS = --branch-probabilities --all-blocks

PHONY += lib docs exes tests cov
lib: $(LIB_TGT)
docs: $(DOC_TGT)
exes: $(EXE_TGT)
tests: $(TEST_TGT)
cov: $(COV_TGT)

$(LIB_TGT): $(LIB_OBJ)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $+

$(TLIB_TGT): $(TLIB_OBJ)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $+

$(EXE_TGT): %: %.o $(LIB_TGT)
	$(LD) $(LDFLAGS) -o $@ $@.o -l$(LIB_NAME)

$(TEST_TGT): %: %.o $(TLIB_TGT)
	$(LD) $(TEST_LDFLAGS) -o $@ $@.o -l$(TLIB_NAME)

vpath %.c $(SRC_DIR) $(EXE_DIR) $(TEST_DIR)

$(EXE_OBJ): $(BUILD_DIR)/%.o: %.c $(LIB_INC) $(EXE_INC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB_OBJ): $(BUILD_DIR)/%.o: %.c $(LIB_INC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TLIB_OBJ) $(TEST_OBJ): $(BUILD_DIR)/test_%.o: %.c $(LIB_INC) | $(BUILD_DIR)
	rm -f $(patsubst %.o, %.gcda, $@) $(patsubst %.o, %.gcno, $@)
	$(CC) $(TEST_CFLAGS) -c -o $@ $<

$(COV_TGT): $(BUILD_DIR)/%.c.gcov: $(BUILD_DIR)/test_%.o run-tests
	$(GCOV) $(GCOVFLAGS) --stdout $< > $@

run-tests: $(TEST_TGT)
	@for i in $(TEST_TGT) ; do \
		echo ./$$i ;\
		./$$i >/dev/null || exit $$? ;\
	done

$(DOC_TGT): $(DOC_CFG) $(DOC_SRC) | $(BUILD_DIR)
	doxygen

$(BUILD_DIR):
	mkdir -p $@

PHONY += clean
clean:
	rm -rf build

PHONY += help
help:
	@echo "Available phony targets:" $(PHONY)

.PHONY: $(PHONY)
