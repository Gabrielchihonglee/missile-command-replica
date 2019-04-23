OBJS	= main.o list.o thread.o scheduler.o sleeper.o
SOURCE	= main.c list.c thread.c scheduler.c sleeper.c
HEADER	= list.h thread.h scheduler.h sleeper.h
OUT	= threading
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 =

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) main.c

list.o: list.c
	$(CC) $(FLAGS) list.c

thread.o: thread.c
	$(CC) $(FLAGS) thread.c

scheduler.o: scheduler.c
	$(CC) $(FLAGS) scheduler.c

sleeper.o: sleeper.c
	$(CC) $(FLAGS) sleeper.c


clean:
	rm -f $(OBJS) $(OUT)
