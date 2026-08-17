PKGS      := gtk+-3.0 libcurl
CC        ?= gcc
CFLAGS    += -std=gnu11 -Wall -Wextra -O2 $(shell pkg-config --cflags $(PKGS))
LDFLAGS   += $(shell pkg-config --libs $(PKGS))

SRC_DIR   := src
BUILD_DIR := build
SOURCES   := $(wildcard $(SRC_DIR)/*.c)
OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET    := debts

.PHONY: all clean install run

all: $(TARGET)

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
