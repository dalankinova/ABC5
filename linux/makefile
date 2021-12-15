TARGET = threademu
CC = g++

all: $(TARGET)
clean:
	rm -f $(TARGET) *.o
	
$(TARGET): threademu.cpp
	$(CC) -o $(TARGET) threademu.cpp -lpthread
