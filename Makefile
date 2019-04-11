OBJS	= missile-command.o functions.o start.o prep.o game.o
SOURCE	= missile-command.c functions.c start.c prep.c game.c
HEADER	= functions.h start.h prep.h game.h
OUT	= missile-command
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = -lm -lncurses -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

missile-command.o: missile-command.c
	$(CC) $(FLAGS) missile-command.c

functions.o: functions.c
	$(CC) $(FLAGS) functions.c

start.o: start.c
	$(CC) $(FLAGS) start.c

prep.o: prep.c
	$(CC) $(FLAGS) prep.c

game.o: game.c
	$(CC) $(FLAGS) game.c


clean:
	rm -f $(OBJS) $(OUT)
