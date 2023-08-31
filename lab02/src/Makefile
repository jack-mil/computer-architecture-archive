SIM_SRC  = sim.cpp pipeline.cpp bpred.cpp
SIM_OBJS = $(SIM_SRC:.cpp=.o)

all: $(SIM_SRC) sim

%.o: %.cpp
	g++ -std=c++14 -O3 -c -o $@ $<

sim: $(SIM_OBJS)
	g++ -std=c++14 -O3 -o $@ $^

clean:
	rm -f sim *.o

clean-all:
	rm -f sim *.o ../scripts/report.txt ../results/*