#include <GL/glew.h>

GLuint loadShader(const char* path, GLenum shaderType);
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);