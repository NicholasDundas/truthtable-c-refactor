TARGET  := truthtable

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := .

EXE := $(BIN_DIR)/$(TARGET)
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC         := clang
CPPFLAGS := -Iinclude -MMD -MP
SANITIZERS := -fsanitize=address $(if $(findstring clang,$(CC)),-fsanitize=undefined)
CFLAGS   := -g -std=c11 -Wall -Wvla -Werror -Wconversion -Wpedantic $(SANITIZERS)
LDFLAGS  := -fsanitize=address $(if $(findstring clang,$(CC)),-fsanitize=undefined)
LDLIBS   := -lm

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)