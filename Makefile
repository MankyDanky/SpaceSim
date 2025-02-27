CXX = g++
GLEW_PATH = /opt/homebrew/opt/glew
GLFW_PATH = /opt/homebrew/opt/glfw
GLM_PATH = /opt/homebrew/opt/glm
CXXFLAGS = -std=c++11 -Wall -I$(GLEW_PATH)/include -I$(GLFW_PATH)/include -I$(GLM_PATH)/include
LDFLAGS = -L$(GLEW_PATH)/lib -L$(GLFW_PATH)/lib -lglfw -framework OpenGL -lGLEW

main: main.o
	$(CXX) -o main main.o $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f main main.o