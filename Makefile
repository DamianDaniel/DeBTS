CC        ?= gcc

# picks whichever webkit2gtk is actually installed, 4.1 first
WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.1 2>/dev/null && echo webkit2gtk-4.1)
ifeq ($(WEBKIT_PKG),)
WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.0 2>/dev/null && echo webkit2gtk-4.0)
endif

REQUIRED_PKGS := gtk+-3.0 libcurl
PKGS          := $(REQUIRED_PKGS) $(WEBKIT_PKG)

# libpq often has no .pc file - fall back to pg_config
LIBPQ_CFLAGS := $(shell pkg-config --exists libpq 2>/dev/null && pkg-config --cflags libpq)
LIBPQ_LIBS   := $(shell pkg-config --exists libpq 2>/dev/null && pkg-config --libs libpq)
ifeq ($(LIBPQ_LIBS),)
PG_CONFIG := $(shell command -v pg_config 2>/dev/null)
ifneq ($(PG_CONFIG),)
LIBPQ_CFLAGS := -I$(shell pg_config --includedir)
LIBPQ_LIBS   := -L$(shell pg_config --libdir) -lpq
endif
endif

CFLAGS    += -std=gnu11 -Wall -Wextra -O2 $(shell pkg-config --cflags $(PKGS) 2>/dev/null) $(LIBPQ_CFLAGS)
LDFLAGS   += $(shell pkg-config --libs $(PKGS) 2>/dev/null) $(LIBPQ_LIBS)

SRC_DIR   := src
BUILD_DIR := build
SOURCES   := $(wildcard $(SRC_DIR)/*.c)
OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET    := debts

.PHONY: all clean install run checkdeps

all: checkdeps $(TARGET)

# fails early with a clear message instead of a wall of header errors
checkdeps:
	@for p in $(REQUIRED_PKGS); do \
		pkg-config --exists $$p || { \
			echo "Missing dependency: $$p (install its -dev/-devel package)"; exit 1; \
		}; \
	done
	@if [ -z "$(WEBKIT_PKG)" ]; then \
		echo "Missing dependency: webkit2gtk-4.1 (or 4.0)"; \
		echo "  Debian/Ubuntu: sudo apt install libwebkit2gtk-4.1-dev"; \
		echo "  Fedora:        sudo dnf install webkit2gtk4.1-devel"; \
		echo "  Arch:          sudo pacman -S webkit2gtk-4.1"; \
		exit 1; \
	fi
	@if [ -z "$(LIBPQ_LIBS)" ]; then \
		echo "Missing dependency: libpq (PostgreSQL client library)"; \
		echo "  Debian/Ubuntu: sudo apt install libpq-dev"; \
		echo "  Fedora:        sudo dnf install libpq-devel"; \
		echo "  Arch:          sudo pacman -S postgresql-libs"; \
		exit 1; \
	fi

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
	install -Dm644 data/style.css $(DESTDIR)/usr/share/debts/style.css

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
