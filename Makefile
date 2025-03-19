CXX = g++
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra -I.
CXXFLAGS_RELEASE = $(CXXFLAGS_COMMON) -O2
CXXFLAGS_DEBUG = $(CXXFLAGS_COMMON) -g -O0 -DDEBUG -fsanitize=address -fno-omit-frame-pointer
LDFLAGS_COMMON = -L./utf8proc -lutf8proc
LDFLAGS_DEBUG = $(LDFLAGS_COMMON) -fsanitize=address

# Source files from different directories
SRCS = main.cpp stemmer.cpp index.cpp \
       include/custom_string.cpp \
       hash/HashBlob.cpp hash/HashTable.cpp hash/HashFile.cpp hash/Common.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = se
TARGET_DEBUG = se_debug

all: release

release: CXXFLAGS = $(CXXFLAGS_RELEASE)
release: LDFLAGS = $(LDFLAGS_COMMON)
release: $(TARGET)

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: LDFLAGS = $(LDFLAGS_DEBUG)
debug: $(TARGET_DEBUG)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(TARGET_DEBUG): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET_DEBUG) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET_DEBUG)

.PHONY: all clean release debug 