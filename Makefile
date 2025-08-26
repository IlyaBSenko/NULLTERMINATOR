# NULL TERMINATOR — Makefile (mac-friendly, works with pkg-config or brew fallback)

APP := null_terminator

# pick up C files in root and/or src/
SRC := $(wildcard src/*.c) $(wildcard *.c)
BIN := bin/$(APP)

CFLAGS := -std=c99 -O2 -Wall

# Try pkg-config first (preferred)
PKG := $(shell pkg-config --cflags --libs raylib 2>/dev/null)

# If pkg-config didn't return anything, use Homebrew fallback (macOS)
ifeq ($(PKG),)
  RAY_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
  ifneq ($(RAY_PREFIX),)
    CFLAGS += -I$(RAY_PREFIX)/include
    LDFLAGS := -L$(RAY_PREFIX)/lib -lraylib \
               -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
  else
    $(warning Could not find raylib via pkg-config or Homebrew. Ensure raylib is installed.)
    LDFLAGS := -lraylib
  endif
else
  LDFLAGS := $(PKG)
endif

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin
	cc $(SRC) -o $(BIN) $(CFLAGS) $(LDFLAGS)
	@echo "Built -> $(BIN)"

run: all
	./$(BIN)

clean:
	rm -rf bin

