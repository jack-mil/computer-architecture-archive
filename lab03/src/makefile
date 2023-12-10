SIM_SRC  = rat.cpp rob.cpp pipeline.cpp sim.cpp exeq.cpp cache.cpp
SIM_OBJS = $(SIM_SRC:.cpp=.o)

all: $(SIM_SRC) sim

%.o: %.cpp
	g++ -g -std=c++14 -O3 -Wall -c -o $@ $<

sim: $(SIM_OBJS)
	g++ -g -std=c++14 -O3 -Wall -o $@ $^

clean:
	rm sim *.o

submit:

