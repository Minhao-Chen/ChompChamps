CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lrt

SRC_DIR = src
BIN_DIR = bin

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/master: $(SRC_DIR)/master.c $(SRC_DIR)/common.h $(SRC_DIR)/ipc_utils.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/view: $(SRC_DIR)/view.c $(SRC_DIR)/common.h $(SRC_DIR)/ipc_utils.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/player: $(SRC_DIR)/player.c $(SRC_DIR)/common.h $(SRC_DIR)/ipc_utils.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

all: $(BIN_DIR)/master $(BIN_DIR)/view $(BIN_DIR)/player

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean