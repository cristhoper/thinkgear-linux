APP=TestBluetooth
CC=gcc
CFLAGS=-g
LIBS=-lbluetooth
SRC= ThinkGearStreamParser.c TestBluetooth.c
OBJ=ThinkGearStreamParser.o TestBluetooth.o

%.o: %.c
	$(CC) -c $< -o $@

$(APP): $(OBJ)
	$(CC) $(CFLAGS) -o $(APP) $(OBJ) $(LIBS)
	
clean:
	rm *.o $(APP)
