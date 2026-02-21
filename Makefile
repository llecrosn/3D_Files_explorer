CC = gcc
CFLAGS = -Wall -Wextra -I./include -g
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
SRC_DIR = src
OBJ_DIR = obj
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = bureau_3d
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
run: all
	./$(TARGET)