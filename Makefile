# Generic Makefile for SpaceSim
# Configure these paths according to your system setup:
GLEW_PATH ?= 
GLFW_PATH ?= 
GLM_PATH ?= 
IMGUI_PATH = vendor/imgui

# Use environment variables if available, otherwise use system defaults
CXX = g++
CXXFLAGS = -std=c++11 -Wall 
LDFLAGS = -lglfw -lGLEW -lGL

# Add paths if specified
ifdef GLEW_PATH
    CXXFLAGS += -I$(GLEW_PATH)/include
    LDFLAGS += -L$(GLEW_PATH)/lib
endif

ifdef GLFW_PATH
    CXXFLAGS += -I$(GLFW_PATH)/include
    LDFLAGS += -L$(GLFW_PATH)/lib
endif

ifdef GLM_PATH
    CXXFLAGS += -I$(GLM_PATH)/include
endif

# Include ImGui path
CXXFLAGS += -I$(IMGUI_PATH)

# Your project source files
PROJECT_SOURCES = main.cpp texture.cpp planet.cpp objects.cpp shader.cpp
PROJECT_OBJECTS = $(PROJECT_SOURCES:.cpp=.o)

# ImGui source files
IMGUI_SOURCES = $(IMGUI_PATH)/imgui.cpp \
                $(IMGUI_PATH)/imgui_draw.cpp \
                $(IMGUI_PATH)/imgui_tables.cpp \
                $(IMGUI_PATH)/imgui_widgets.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_opengl3.cpp

# ImGui object files
IMGUI_OBJECTS = $(IMGUI_SOURCES:.cpp=.o)

# All object files combined
ALL_OBJECTS = $(PROJECT_OBJECTS) $(IMGUI_OBJECTS)

# Default target
main: $(ALL_OBJECTS)
    $(CXX) -o main $(ALL_OBJECTS) $(LDFLAGS)

# Rule for project source files
%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
    rm -f main $(ALL_OBJECTS)

# Platform-specific adjustments
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -framework OpenGL
endif

# Help target
help:
    @echo "SpaceSim Build System"
    @echo "--------------------"
    @echo "Available targets:"
    @echo "  main   - Build the SpaceSim executable"
    @echo "  clean  - Remove built files"
    @echo "  help   - Display this help message"
    @echo ""
    @echo "Configuration:"
    @echo "  Set these variables according to your setup:"
    @echo "  GLEW_PATH - Path to GLEW installation (e.g., /opt/homebrew/opt/glew)"
    @echo "  GLFW_PATH - Path to GLFW installation (e.g., /opt/homebrew/opt/glfw)"
    @echo "  GLM_PATH  - Path to GLM installation (e.g., /opt/homebrew/opt/glm)"
    @echo ""
    @echo "Example:"
    @echo "  make GLEW_PATH=/usr/local GLFW_PATH=/usr/local GLM_PATH=/usr/local"