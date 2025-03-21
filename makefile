# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -Werror -D_GNU_SOURCE -Ilibs/cJSON

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Executable names
TARGETS = $(BIN_DIR)/direttore $(BIN_DIR)/erogatore_ticket $(BIN_DIR)/operatore $(BIN_DIR)/utente $(BIN_DIR)/sportello

# cJSON source and object
CJSON_SRC = libs/cJSON/cJSON.c
CJSON_OBJ = $(BUILD_DIR)/cJSON.o

# Common source files (shared across executables)
COMMON_SRCS = $(SRC_DIR)/memory_handler.c $(SRC_DIR)/config.c
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(COMMON_SRCS))

# Source files for each executable
SRCS_direttore = $(SRC_DIR)/direttore.c $(COMMON_SRCS)
SRCS_erogatore = $(SRC_DIR)/erogatore_ticket.c $(COMMON_SRCS)
SRCS_operatore = $(SRC_DIR)/operatore.c $(COMMON_SRCS)
SRCS_utente = $(SRC_DIR)/utente.c $(COMMON_SRCS)
SRCS_sportello = $(SRC_DIR)/sportello.c $(COMMON_SRCS)

# Object files for each executable
OBJS_direttore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_direttore)) $(CJSON_OBJ)
OBJS_erogatore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_erogatore)) $(CJSON_OBJ)
OBJS_operatore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_operatore)) $(CJSON_OBJ)
OBJS_utente = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_utente)) $(CJSON_OBJ)
OBJS_sportello = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_sportello)) $(CJSON_OBJ)

# Default rule: Build all executables
all: $(TARGETS)

# Compile each executable separately
$(BIN_DIR)/direttore: $(OBJS_direttore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_direttore) -o $@

$(BIN_DIR)/erogatore_ticket: $(OBJS_erogatore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_erogatore) -o $@

$(BIN_DIR)/operatore: $(OBJS_operatore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_operatore) -o $@

$(BIN_DIR)/utente: $(OBJS_utente) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_utente) -o $@

$(BIN_DIR)/sportello: $(OBJS_sportello) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_sportello) -o $@

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for compiling cJSON object file
$(BUILD_DIR)/cJSON.o: $(CJSON_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build and bin directories if needed
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean generated files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run the main simulation program (Direttore)
run: all
	./$(BIN_DIR)/direttore
