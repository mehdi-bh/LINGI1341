# You can use clang if you prefer
CC = gcc 

# Feel free to add other C flags
CFLAGS += -c -std=gnu99 -Wall -Werror -Wextra -O2 
# By default, we colorize the output, but this might be ugly in log files, so feel free to remove the following line.
CFLAGS += -D_COLOR 

# You may want to add something here
LDFLAGS += -lz

# Adapt these as you want to fit with your project
SENDER_SOURCES = $(wildcard src/sender.c src/logs/log.c src/buffer/buffer.c src/packet/packet.c src/socket/socket_manager.c )
RECEIVER_SOURCES = $(wildcard src/receiver.c src/logs/log.c src/buffer/buffer.c src/packet/packet.c src/socket/socket_manager.c) 

SENDER_OBJECTS = $(SENDER_SOURCES:.c=.o)
RECEIVER_OBJECTS = $(RECEIVER_SOURCES:.c=.o)

SENDER = sender
RECEIVER = receiver

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER_OBJECTS)
	$(CC) $(SENDER_OBJECTS) -o $@ $(LDFLAGS)

$(RECEIVER): $(RECEIVER_OBJECTS)
	$(CC) $(RECEIVER_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean
clean:
	@cd tests && $(MAKE) -s clean
	rm -f $(SENDER_OBJECTS) $(RECEIVER_OBJECTS)
	rm -f $(SENDER) $(RECEIVER)
	rm -f "received_file" "sender.log" "receiver.log" "input_file"

# Tests
create_tests:
	@cd tests && $(MAKE) -s
	@cd tests && $(MAKE) -s run

tests_sh:
	./tests/run_tests.sh

tests: all create_tests tests_sh

# Debug
debug: CFLAGS += -D_DEBUG
debug: clean all

### SUBMISSION ###
# Place the zip in the parent repository of the project
ZIP_NAME="../projet1_benhaddou_hennebo.zip"

# A zip target, to help you have a proper zip file. You probably need to adapt this code.
zip:
	# Generate the log file stat now. Try to keep the repository clean.
	git log --stat > gitlog.stat
	zip -r $(ZIP_NAME) Makefile src tests rapport.pdf gitlog.stat
	# We remove it now, but you can leave it if you want.
	rm gitlog.stat
