OBJS	= missile-command.o functions.o start.o prep.o game.o end.o threading/list.o threading/thread.o threading/scheduler.o threading/sleeper.o threading/input.o
SOURCE	= missile-command.c functions.c start.c prep.c game.c end.c threading/list.c threading/thread.c threading/scheduler.c threading/sleeper.c threading/input.c
HEADER	= functions.h start.h prep.h game.h end.h threading/list.h threading/thread.h threading/scheduler.h threading/sleeper.h threading/input.h
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

end.o: end.c
	$(CC) $(FLAGS) end.c

threading/list.o: threading/list.c
	$(CC) $(FLAGS) threading/list.c -o threading/list.o

threading/thread.o: threading/thread.c
	$(CC) $(FLAGS) threading/thread.c -o threading/thread.o

threading/scheduler.o: threading/scheduler.c
	$(CC) $(FLAGS) threading/scheduler.c -o threading/scheduler.o

threading/sleeper.o: threading/sleeper.c
	$(CC) $(FLAGS) threading/sleeper.c -o threading/sleeper.o

threading/input.o: threading/input.c
	$(CC) $(FLAGS) threading/input.c -o threading/input.o


clean:
	rm -f $(OBJS) $(OUT)
