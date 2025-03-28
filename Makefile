CXX = g++
GLEW_PATH = /opt/homebrew/opt/glew
GLFW_PATH = /opt/homebrew/opt/glfw
GLM_PATH = /opt/homebrew/opt/glm
IMGUI_PATH = vendor/imgui
CXXFLAGS = -std=c++11 -Wall -I$(GLEW_PATH)/include -I$(GLFW_PATH)/include -I$(GLM_PATH)/include -I$(IMGUI_PATH)
LDFLAGS = -L$(GLEW_PATH)/lib -L$(GLFW_PATH)/lib -lglfw -framework OpenGL -lGLEW

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