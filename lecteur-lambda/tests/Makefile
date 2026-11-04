# Tests sur PC du cœur portable (table Bosch, conversions, chauffe).
#   make            compile et lance les tests
#   make trace      écrit le cycle de chauffe simulé (CSV) et son graphe (SVG) dans ../docs
CXX ?= g++
# -std=c++11 : même norme que le compilateur de l'Arduino Mega.
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Werror -Wshadow
SRC_DIR := ../LecteurLambda
CORE := $(SRC_DIR)/lsu_adv.cpp $(SRC_DIR)/heater.cpp $(SRC_DIR)/sensor_sim.cpp
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
