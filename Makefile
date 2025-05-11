ifeq ($(OS), Windows_NT)
	LDFLAGS = -luser32 -lgdi32
	EXT = .exe
endif

TARGET = traffic$(EXT)

$(TARGET): traffic.c traffic.h tests.h
	gcc $(LDFLAGS) -o $@ traffic.c

gui.exe: gui.c drawing.h traffic.c traffic.h
	gcc gui.c -o gui.exe $(LDFLAGS) -Wl,-subsystem,console

test: $(TARGET)
	./$(TARGET) --test
