SOURCE  := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,%.o,$(SOURCE))

TARGET  := roughserver
CC      := g++
LIBS    := -lpthread
CFLAGS  := -std=c++11 -g -Wall -O3
CXXFLAGS:= $(CFLAGS)

.PHONY : objs clean veryclean rebuild debug
objs : $(OBJS)
rebuild: veryclean

clean :
	find . -name '*.o' | xargs rm -f
veryclean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f
debug:
	@echo $(SOURCE)

$(TARGET) : $(OBJS) Main.o
	$(CC) $(CXXFLAGS) -o $@ $^ $(LIBS)