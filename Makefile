CC := gcc
CFLAGS := -std=gnu18 -Wall -Wextra -Werror -pedantic -O2

VFLAGS := --leak-check=full --show-leak-kinds=all  --track-origins=yes

SRC := paclist.c package.c osx-handle.c
SRC := $(addprefix src/, $(SRC))

TARGET := paclist

BUILDDIR = build

OBJ = $(patsubst %.c,$(RELEASEDIR)/%.o,$(SRC))
OBJ-dbg = $(patsubst %.c,$(DEBUGDIR)/%.o,$(SRC))

all: run

$(TARGET): $(SRC)
	@$(CC) $(CFLAGS) $^ -lncurses -o $@

$(RELEASEDIR) $(DEBUGDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	@./$^

leaks:
	valgrind $(VFLAGS) ./a.out
