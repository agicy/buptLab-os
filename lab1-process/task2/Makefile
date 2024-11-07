CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -O2
SOLUTION_FLAGS := -std=c++20 -Wall -Wextra -Werror -O0
LDFLAGS :=
SOURCES := organizer.cpp student.cpp solution.cpp
OBJECTS := $(SOURCES:.cpp=.o)
EXECUTABLE_STUDENT := student
EXECUTABLE_ORGANIZER := organizer

build: $(EXECUTABLE_STUDENT) $(EXECUTABLE_ORGANIZER)

solution.o: solution.cpp
	$(CXX) $(SOLUTION_FLAGS) -c solution.cpp -o solution.o

$(EXECUTABLE_ORGANIZER): organizer.o solution.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) organizer.o solution.o -o $(EXECUTABLE_ORGANIZER)

$(EXECUTABLE_STUDENT): student.o solution.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) student.o solution.o -o $(EXECUTABLE_STUDENT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE_STUDENT) $(EXECUTABLE_ORGANIZER)

run: $(EXECUTABLE_ORGANIZER) $(EXECUTABLE_STUDENT)
	./$(EXECUTABLE_ORGANIZER)

.PHONY: build clean run
