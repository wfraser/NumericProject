CXXFLAGS+=-std=c++14 -Wall -pedantic
LDFLAGS+=-Wall
LDLIBS+=-lstdc++

OBJS=NumericProject.o

NumericProject: $(OBJS)
	$(CXX) $(LDFLAGS) -o NumericProject $(OBJS) $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.o NumericProject
