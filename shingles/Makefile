CXX = g++
CXXFLAGS = -std=c++14 -Wall -g -O0 -I../../include $(shell pkg-config --cflags openssl)
LDFLAGS = $(shell pkg-config --libs openssl)

# Source and output
SRC = testShingles.cpp ../../parser/HtmlParser.cpp ../../Crawler/crawler.cpp ../../parser/HtmlTags.cpp shingle.cpp ../../include/cf/searchstring.cpp
OBJ = $(addprefix ../../parser/, HtmlParser.o HtmlTags.o) $(addprefix ../../Crawler/, crawler.o) $(addprefix ../../include/cf/, searchstring.o) testShingles.o shingle.o
EXEC = testShingles

# Compilation and linking
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

../../parser/%.o: ../../parser/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../../Crawler/%.o: ../../Crawler/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../../include/cf/%.o: ../../include/cf/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean 