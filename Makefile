all : train gen exec

gen : gen.o nwclass.o rand.o
	c++ -o gen gen.o nwclass.o rand.o

train : train.o nwclass.o rand.o
	c++ -o train train.o nwclass.o rand.o

exec : exec.o nwclass.o rand.o
	c++ -o exec exec.o nwclass.o rand.o

exec.o : exec.c
	c++ -c exec.c

gen.o : gen.c
	c++ -c gen.c

train.o : train.c
	c++ -c train.c

nwclass.o : nwclass.cpp nwclass.h
	c++ -c nwclass.cpp

rand.o : rand.cpp
	c++ -c rand.cpp

