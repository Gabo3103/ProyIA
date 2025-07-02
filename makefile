CC = g++
CFLAGS = -Wall -std=c++17
RM = rm -f
NOMBRE = proyecto
SRC = main.cpp
OBJ = $(SRC:.cpp=.o)
INST ?= GA1

all: $(NOMBRE)

$(NOMBRE): $(OBJ)
	$(CC) $(CFLAGS) -o $(NOMBRE) $(OBJ)

run: all
	./$(NOMBRE) $(INST)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(NOMBRE) *.o
