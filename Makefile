SOURCE  := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,%.o,$(SOURCE))

TARGET  := roughserver
CC      := g++
LIBS    := -lpthread
CFLAGS  := -std=c++11 -g -Wall -O3
CXXFLAGS:= $(CFLAGS)

$(TARGET) : $(OBJS) Main.o
	$(CC) $(CXXFLAGS) -o $@ $^ $(LIBS)

.PHONY : objs clean veryclean rebuild debug
objs : $(OBJS)
rebuild : veryclean

clean :
	find . -name '*.o' | xargs rm -f
targetclean :
	find . -name $(TARGET) | xargs rm -f
veryclean : clean targetclean
debug :
	@echo $(SOURCE)