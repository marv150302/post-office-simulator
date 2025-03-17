# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -Werror -D_GNU_SOURCE

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Executable names
TARGETS = $(BIN_DIR)/direttore $(BIN_DIR)/erogatore_ticket $(BIN_DIR)/operatore $(BIN_DIR)/utente $(BIN_DIR)/sportello

# Source files for each executable
SRCS_direttore = $(SRC_DIR)/direttore.c $(SRC_DIR)/config.c
SRCS_erogatore = $(SRC_DIR)/erogatore_ticket.c $(SRC_DIR)/config.c
SRCS_operatore = $(SRC_DIR)/operatore.c $(SRC_DIR)/config.c
SRCS_utente = $(SRC_DIR)/utente.c $(SRC_DIR)/config.c
SRCS_sportello = $(SRC_DIR)/sportello.c $(SRC_DIR)/config.c

# Object files for each executable
OBJS_direttore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_direttore))
OBJS_erogatore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_erogatore))
OBJS_operatore = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_operatore))
OBJS_utente = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_utente))
OBJS_sportello = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_sportello))

# Default rule: Build all executables
all: $(TARGETS)

# Compile each executable separately
$(BIN_DIR)/direttore: $(OBJS_direttore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_direttore) -o $(BIN_DIR)/direttore

$(BIN_DIR)/erogatore_ticket: $(OBJS_erogatore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_erogatore) -o $(BIN_DIR)/erogatore_ticket

$(BIN_DIR)/operatore: $(OBJS_operatore) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_operatore) -o $(BIN_DIR)/operatore

$(BIN_DIR)/utente: $(OBJS_utente) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_utente) -o $(BIN_DIR)/utente

$(BIN_DIR)/sportello: $(OBJS_sportello) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS_sportello) -o $(BIN_DIR)/sportello

# Rule to compile .c files into .o files in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build/ and bin/ directories exist
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
