CC := gcc
CFLAGS := -std=gnu18 -Wall -Wextra -Werror -pedantic -O2

VFLAGS := --leak-check=full --show-leak-kinds=all  --track-origins=yes

SRC := $(wildcard src/*.c)

ifeq ($(shell echo __APPLE__ | $(CC) -E - | grep -c __APPLE__),0)
	SRC += $(shell find src/OSX -name '*.c')
endif

ifeq ($(shell echo __linux__ | $(CC) -E - | grep -c __linux__),0)
	SRC += $(shell find src/ARCH -name '*.c')
endif

TARGET := paclist

BUILDDIR = build

OBJ = $(patsubst %.c,$(RELEASEDIR)/%.o,$(SRC))
OBJ-dbg = $(patsubst %.c,$(DEBUGDIR)/%.o,$(SRC))

all: run

$(TARGET): $(SRC)
	@$(CC) $(CFLAGS) $^ -lncurses -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	@./$^

leaks:
	valgrind $(VFLAGS) ./a.out
