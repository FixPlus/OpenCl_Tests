

SOURCES = main.cpp bitonic.cpp

EXECS = $(SOURCES:.cpp=.o)

all: $(EXECS)

.cpp.o:
	g++ --std=c++2a -o $@ $< -lOpenCL

clear:
	rm -f $(EXECS)

gitCommitAndPush: clear
	git add .
	git commit -m "$(MESSAGE)"
	git push -u origin master