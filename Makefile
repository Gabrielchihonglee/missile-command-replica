OBJS	= missile-command.o
SOURCE	= missile-command.c
HEADER	= 
OUT	= missile-command
CC	= gcc
FLAGS	= -g -c -Wall
LFLAGS	= -lncurses

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

missile-command.o: missile-command.c
	$(CC) $(FLAGS) missile-command.c 

clean:
	rm -f $(OBJS) $(OUT)
