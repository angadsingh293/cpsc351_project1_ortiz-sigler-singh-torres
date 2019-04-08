# C++ compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall -Wpedantic

# To make multiple executibles
all: receiver sender

# Build receiver
receiver: recv.cpp msg.h
	$(CC) $(CFLAGS) -cpp recv.cpp -o recv

# Build sender
sender: sender.cpp msg.h
	$(CC) $(CFLAGS) -cpp sender.cpp -o sender

# Clean up
clean:
	rm -f core recv sender
