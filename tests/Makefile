CC = gcc

# options
CFLAGS += -g -W -lz -lcunit

# sources
SRC = buffer_tests.o packet_tests.o

# make
all: clean $(SRC)

# tests
buffer_tests.o: buffer_tests.c
	@echo 'Building buffer_tests'
	@$(CC) -c -o buffer_tests.o buffer_tests.c $(CFLAGS) $(LDFLAGS)
	@$(CC) buffer_tests.o ../src/packet/packet.o ../src/buffer/buffer.o -o buffer_tests $(CFLAGS) $(LDFLAGS)

packet_tests.o: packet_tests.c
	@echo 'Building packet_tests'
	@$(CC) -c -o packet_tests.o packet_tests.c $(CFLAGS) $(LDFLAGS)
	@$(CC) packet_tests.o ../src/packet/packet.o -o packet_tests $(CFLAGS) $(LDFLAGS)

# clean files
clean:
	@rm -vf *.o buffer_tests packet_tests

# run tests
run:
	./buffer_tests
	./packet_tests