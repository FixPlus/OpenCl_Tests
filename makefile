

all:
	g++ --std=c++2a main.cpp -lOpenCL -o oclTest

clear:
	rm oclTest