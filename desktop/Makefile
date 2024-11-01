# Compiler
CXX = clang++

# Compiler flags
CXXFLAGS = \
	-Wall \
	-Wextra \
	-std=c++17 \
	`sdl2-config --cflags` \
	-I/opt/homebrew/Cellar/sdl2/2.30.8/include \
	-I../firmware/src/Emulator \
	-I../firmware/src/AudioOutput \
	-I../firmware/src/Emulator/z80

LDFLAGS = -L/usr/local/lib -lSDL2

# Target executable name
TARGET = sdl_app

# Source files
SRCS = \
  ../firmware/src/Emulator/128k_rom.cpp \
  ../firmware/src/Emulator/48k_rom.cpp \
  ../firmware/src/Emulator/spectrum.cpp \
  ../firmware/src/Emulator/z80/z80.cpp \
  ../firmware/src/AySound/AySound.cpp \
  src/main.cpp \
  src/tzx_cas.cpp

# Object files (object files will have the same path as source files, but with .o extension)
OBJS = $(SRCS:.cpp=.o)

# Dependency files
DEPS = $(OBJS:.o=.d)

# Default rule
all: $(TARGET)

# Create executable from object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) `sdl2-config --libs`

# Object file rules (object files are placed next to the source files)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean up build files
clean:
	rm -f $(SRCS:.cpp=.o) $(SRCS:.cpp=.d) $(TARGET)

# Phony targets
.PHONY: all clean
