

SOURCES = main.cpp bitonic.cpp

EXECS = $(SOURCES:.cpp=.o)

all: $(EXECS)

.cpp.o:
	g++ --std=c++2a -o $@ $< -lOpenCL

clear:
	rm $(EXECS)