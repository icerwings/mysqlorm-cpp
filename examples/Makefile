

PRG = benchmark orm_test

CC = g++
CXXFLAG = -g -fPIC -Wall -Wno-reorder -std=c++11 -lpthread
SRC = $(wildcard ./test_orm.cpp)
OBJ = $(SRC:%.cpp=%.o)

INC=-I. -I../src -I/usr/local/mysql/include
LIB=-L/usr/local/mysql/lib -lmysqlclient

all : $(PRG)

orm_test : test_orm.o
	$(CC) $(CXXFLAG) -o $@ $^ $(LIB) $(INC)

benchmark : benchmark.o
	$(CC) $(CXXFLAG) -o $@ $^ $(LIB) $(INC)

%.o : %.cpp
	$(CC) $(CXXFLAG) -c $< -o $@ $(INC)

clean :
	rm $(OBJ) $(PRG)

