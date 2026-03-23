CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

#TODO - lj,fdbnm dfhbfwb. lkz dbyls
ifeq ($(OS),Windows_NT) 
RM_CMD := cmd /c del /f /q
else
RM_CMD := rm -f
endif

INCLUDES := \
	-I. \
	-Idynamic_array/include \
	-Isingly_linked_list/include \
	-Istack_array/include \
	-Istack_list/include \
	-Ibenchmark/include

COMMON_SOURCES := \
	logger.c \
	dynamic_array/source/dynamic_array.c \
	singly_linked_list/source/singly_linked_list.c \
	stack_array/source/stack_array.c \
	stack_list/source/stack_list.c

BENCHMARK_SOURCES := \
	$(COMMON_SOURCES) \
	benchmark/source/benchmark.c \
	main.c

TEST_SOURCES := \
	$(COMMON_SOURCES) \
	benchmark/source/benchmark.c \
	tests/test_api.c

TARGET := benchmark_app
TEST_TARGET := api_tests

.PHONY: all test clean

all: $(TARGET)

test: $(TEST_TARGET)

$(TARGET): $(BENCHMARK_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(BENCHMARK_SOURCES) -o $(TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	-$(RM_CMD) $(TARGET) $(TARGET).exe
	-$(RM_CMD) $(TEST_TARGET) $(TEST_TARGET).exe
	-$(RM_CMD) target target.exe
