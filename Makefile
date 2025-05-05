ifeq ($(OS), Windows_NT)
	LDFLAGS = -luser32
	EXT = .exe
endif

TARGET = traffic$(EXT)

$(TARGET): traffic.c traffic.h tests.h
	gcc $(LDFLAGS) -o $@ traffic.c

test: $(TARGET)
	./$(TARGET) --test
