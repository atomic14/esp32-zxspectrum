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
	-I../firmware/src/Emulator/z80 \
	-I../firmware/src/TZX

# Linker flags (including Cocoa framework for macOS file picker support)
LDFLAGS = -L/usr/local/lib -framework Cocoa

# Target executable name
TARGET = sdl_app

SRCS = \
  ../firmware/src/Emulator/128k_rom.cpp \
  ../firmware/src/Emulator/48k_rom.cpp \
  ../firmware/src/Emulator/spectrum.cpp \
  ../firmware/src/Emulator/z80/z80.cpp \
  ../firmware/src/AySound/AySound.cpp \
	../firmware/src/TZX/tzx_cas.cpp

OBJC_SRCS = \
	src/main.mm

# Object files (object files will have the same path as source files, but with .o extension)
OBJS = $(SRCS:.cpp=.o) $(OBJC_SRCS:.mm=.o)

# Dependency files
DEPS = $(OBJS:.o=.d)

# Default rule
all: $(TARGET)

# Create executable from object files
$(TARGET): $(OBJS) Makefile
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) `sdl2-config --libs` $(LDFLAGS)

# Object file rules (object files are placed next to the source files)
%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

# Rules for .mm files (for Objective-C++)
%.o: %.mm Makefile
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

# Include dependency files
-include $(DEPS)

# Clean up build files
clean:
	rm -f $(SRCS:.cpp=.o) $(SRCS:.cpp=.d) $(OBJC_SRCS:.mm=.o) $(OBJC_SRCS:.mm=.d) $(TARGET)

# Phony targets
.PHONY: all clean
