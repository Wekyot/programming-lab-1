ifeq ($(OS),Windows_NT)
    RM = del /Q /F
    TARGET_EXTENSION = .exe
else
    RM = rm -f
    TARGET_EXTENSION =
endif

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c17

# Имя вашей готовой программы
TARGET = csvstat

# Автоматически находим все файлы .c в текущей директории
SRCS = $(wildcard *.c)

# Превращаем список .c файлов в список .o файлов (main.c -> main.o)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

# Правило по умолчанию
all: $(TARGET)

# Сборка программы из объектных файлов
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Универсальное правило для сборки любого .o файла из .c файла
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка мусора
clean:
	$(RM) *.o csvstat$(TARGET_EXTENSION)
