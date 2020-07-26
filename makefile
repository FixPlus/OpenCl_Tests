

SOURCES = vecadd.cpp bitonic.cpp matrices.cpp

EXECS = $(SOURCES:.cpp=.o)

all: $(EXECS)

.cpp.o:
	g++ --std=c++2a -o $@ $< -lOpenCL

clear:
	rm -f $(EXECS)

gitCommit: clear
	git add .
	git commit -m "$(MESSAGE)"

gitPush:
	git push -u origin master

gitCommitAndPush: gitCommit 
	gitPush
