# Tests sur PC du cœur portable (table Bosch, pilotage direct, chauffe) sur la sonde simulée.
#   make            compile et lance les tests
#   make trace      écrit le cycle simulé (CSV) et son graphe (SVG) dans ../docs
CXX ?= g++
# -std=c++11 : le code n'utilise que cette norme.
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Werror -Wshadow
SRC_DIR := ../LecteurLambda
CORE := $(SRC_DIR)/lsu_adv.cpp $(SRC_DIR)/heater.cpp $(SRC_DIR)/lsu_driver.cpp $(SRC_DIR)/sim_hardware.cpp
HEADERS := $(wildcard $(SRC_DIR)/*.h)

all: test

test: test_core
	./test_core

test_core: test_core.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -o $@ test_core.cpp $(CORE) -lm

trace_chauffe: trace_chauffe.cpp $(CORE) $(HEADERS)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -o $@ trace_chauffe.cpp $(CORE) -lm

trace: trace_chauffe
	./trace_chauffe > ../docs/cycle-chauffe-simule.csv
	python3 trace_svg.py ../docs/cycle-chauffe-simule.csv ../docs/cycle-chauffe-simule.svg

clean:
	rm -f test_core trace_chauffe

.PHONY: all test trace clean
