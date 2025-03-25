CXX = g++
GLEW_PATH = /opt/homebrew/opt/glew
GLFW_PATH = /opt/homebrew/opt/glfw
GLM_PATH = /opt/homebrew/opt/glm
IMGUI_PATH = vendor/imgui
CXXFLAGS = -std=c++11 -Wall -I$(GLEW_PATH)/include -I$(GLFW_PATH)/include -I$(GLM_PATH)/include -I$(IMGUI_PATH)
LDFLAGS = -L$(GLEW_PATH)/lib -L$(GLFW_PATH)/lib -lglfw -framework OpenGL -lGLEW

# ImGui source files
IMGUI_SOURCES = $(IMGUI_PATH)/imgui.cpp \
				$(IMGUI_PATH)/imgui_draw.cpp \
                $(IMGUI_PATH)/imgui_tables.cpp \
                $(IMGUI_PATH)/imgui_widgets.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_opengl3.cpp

# Object files
IMGUI_OBJECTS = $(IMGUI_SOURCES:.cpp=.o)

main: main.o $(IMGUI_OBJECTS)
	$(CXX) -o main main.o $(IMGUI_OBJECTS) $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

# Generic rule for ImGui object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f main main.o $(IMGUI_OBJECTS)