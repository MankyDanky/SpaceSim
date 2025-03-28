#include <GL/glew.h>
#include <string>

GLuint loadTexture(const std::string& path);

unsigned int loadCubemap(std::vector<std::string> faces);