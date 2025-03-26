#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_glfw.h"
#include "vendor/imgui/backends/imgui_impl_opengl3.h"

int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

int focusedPlanetIndex = -1; // Default is -1 (sun)
int hoveredPlanetIndex = -1; // Default is -1 (no planet)
bool followingPlanet = false;

glm::mat4 currentView;
glm::mat4 currentProjection;

// Simulation speed control
float timeScale = 1.0f;      // 1.0 = normal speed, 2.0 = double speed, 0.0 = paused
bool isPaused = false;  
GLuint pauseTexture, playTexture, forwardTexture;

// Add these to your global variables
glm::vec3 currentOrbitCenter(0.0f);  // Current camera target position
glm::vec3 targetOrbitCenter(0.0f);   // Target position we're moving towards
float transitionSpeed = 4.0f;        // Speed of camera transition (adjust as needed)
bool inTransition = false;           // Whether we're currently in transition

// 2. Create a helper function to get planet names
std::string getPlanetName(int index) {
    if (index == -1) return "Sun";
    
    switch(index) {
        case 0: return "Sun";
        case 1: return "Mercury";
        case 2: return "Venus";
        case 3: return "Earth";
        case 4: return "Mars";
        case 5: return "Jupiter";
        case 6: return "Saturn";
        case 7: return "Uranus";
        case 8: return "Neptune";
        default: return "Unknown";
    }
}

// Forward declaration for the Planet class
class Planet {
    public:
        // Member variables
        GLuint VAO, VBO, EBO;
        GLuint texture;
        float radius;
        float orbitRadius;
        float rotationSpeed;
        float orbitalSpeed;
        float axialTilt;
        float emissionStrength;
        glm::vec3 position;
        unsigned int indexCount;
        
        // Constructor
        Planet(float radius, float orbitRadius, float rotationSpeed, float orbitalSpeed, 
               float axialTilt, float emissionStrength, GLuint texture);
        
        // Destructor
        ~Planet();
        
        // Update planet position based on orbit
        void update(float currentTime);
        
        // Draw planet
        void draw(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection, 
                  glm::vec3 viewPos, glm::vec3 lightPos);
        
        void drawOutline(GLuint shaderProgram);
    };

// Vector to hold pointers to planets
std::vector<Planet*> planets; // std::vector<Planet> planets;

unsigned int postProcessingFBO, rbo;
unsigned int postProcessingTexture, bloomTexture;
unsigned int skyboxFBO, skyboxTexture;
unsigned int pingpongFBO[2], pingpongBuffer[2];

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

float cameraDistance = 10.0f;
float lastX = 400, lastY = 300; // Initial mouse position
float yaw = 0.0f;               // Horizontal rotation angle
float pitch = 0.0f;             // Vertical rotation angle
bool firstMouse = true;         // First mouse input flag
bool orbitActive = false;       // Orbit mode flag

// Shader loading utility functions
GLuint loadShader(const char* path, GLenum shaderType);
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void recreateFramebuffers(int width, int height) {
    // Clean up existing framebuffers and textures
    glDeleteFramebuffers(1, &postProcessingFBO);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &postProcessingTexture);
    glDeleteTextures(1, &bloomTexture);
    glDeleteFramebuffers(1, &skyboxFBO);
    glDeleteTextures(1, &skyboxTexture);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(2, pingpongBuffer);

    
    

    // Create Frame Buffer Object
    glGenFramebuffers(1, &postProcessingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);
    
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Create main color texture
    glGenTextures(1, &postProcessingTexture);
    glBindTexture(GL_TEXTURE_2D, postProcessingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingTexture, 0);
    
    // Create bloom texture
    glGenTextures(1, &bloomTexture);
    glBindTexture(GL_TEXTURE_2D, bloomTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bloomTexture, 0);
    
    // Create skybox framebuffer
    glGenFramebuffers(1, &skyboxFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, skyboxFBO);
    
    // Create skybox texture
    glGenTextures(1, &skyboxTexture);
    glBindTexture(GL_TEXTURE_2D, skyboxTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyboxTexture, 0);
    
    // Create ping-pong framebuffers for bloom
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Update the global dimensions
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
    
    // Recreate framebuffers with new size (call this function we'll define below)
    recreateFramebuffers(width, height);
}

unsigned int skyboxVAO = 0, skyboxVBO = 0;
void setupSkybox() {
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f
    };

    // Create skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            // Choose format based on channels
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    
    // Use GL_LINEAR_MIPMAP_LINEAR for better quality
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    // Generate mipmaps to reduce artifacts at distance
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return textureID;
}

// Helper function to render a full-screen quad
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions        // texture coordinates
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f
        };
        
        // Setup quad VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        
        // Texture coordinates attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    
    // Render quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void createSphere(float radius, int sectorCount, int stackCount, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // Add an extra vertex at the end of each ring to fix the texture seam
        for(int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            

            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    // generate CCW index list of sphere triangles
    int k1, k2;
    for(int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if(i != (stackCount-1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

// Add this function after createSphere
bool ray_sphere_intersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
    const glm::vec3& sphereCenter, float sphereRadius,
    float& t) {
    glm::vec3 oc = rayOrigin - sphereCenter;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b*b - 4*a*c;

    if (discriminant < 0) {
        return false;
    } else {
        t = (-b - sqrt(discriminant)) / (2.0f * a);
        return t > 0;
    }
}

// Function to find which planet was clicked
int findClickedPlanet(GLFWwindow* window, const glm::mat4& view, const glm::mat4& projection,
    const std::vector<Planet*>& planets) {
    // Get mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Convert screen coordinates to normalized device coordinates
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    float x = (2.0f * mouseX) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / windowHeight;

    // Create ray in NDC space
    glm::vec4 rayStart = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEnd = glm::vec4(x, y, 0.0f, 1.0f);

    // Convert to world space
    glm::mat4 inverseViewProj = glm::inverse(projection * view);
    glm::vec4 worldRayStart = inverseViewProj * rayStart;
    worldRayStart /= worldRayStart.w;
    glm::vec4 worldRayEnd = inverseViewProj * rayEnd;
    worldRayEnd /= worldRayEnd.w;

    // Calculate ray direction
    glm::vec3 rayOrigin = glm::vec3(worldRayStart);
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldRayEnd - worldRayStart));

    // Check intersection with each planet
    int closestPlanet = -1;
    float closestT = FLT_MAX;

    for (int i = 0; i < planets.size(); i++) {
        float t;
        if (ray_sphere_intersection(rayOrigin, rayDir, 
            planets[i]->position, 
            planets[i]->radius, t)) {
            if (t < closestT) {
                closestT = t;
                closestPlanet = i;
            }
        }
    }

    return closestPlanet;
}

void updateHoveredPlanet(GLFWwindow* window, const glm::mat4& view, const glm::mat4& projection) {
    // Only update hover when not orbiting
    if (orbitActive)
        return;
        
    hoveredPlanetIndex = findClickedPlanet(window, view, projection, planets);
}

GLuint loadTexture(const std::string& path) {
    // Debug output
    std::cout << "Loading texture: " << path << std::endl;
    
    // Load image data
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        
        // Create a default color texture for missing files
        unsigned char defaultColor[4] = {255, 0, 0, 255}; // Red
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultColor);
        return textureID;
    }
    
    // Determine format based on channels
    GLenum format;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    else {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    // Create and configure texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Load data into texture
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Free image data
    stbi_image_free(data);
    
    std::cout << "Texture loaded with ID: " << textureID << std::endl;
    return textureID;
}
        
// Constructor
Planet::Planet(float radius, float orbitRadius, float rotationSpeed, float orbitalSpeed, 
        float axialTilt, float emissionStrength, GLuint texture) 
    : radius(radius), orbitRadius(orbitRadius), rotationSpeed(rotationSpeed),
        orbitalSpeed(orbitalSpeed), axialTilt(axialTilt), emissionStrength(emissionStrength),
        position(glm::vec3(orbitRadius, 0.0f, 0.0f)), texture(texture) {
    
    // Create sphere geometry
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    createSphere(radius, 36, 18, vertices, indices);
    indexCount = indices.size();
    
    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Vertex normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Vertex texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    std::cout << "Planet created: VAO=" << VAO << ", texture=" << texture << ", indices=" << indexCount << std::endl;
}
        
        // Destructor
Planet::~Planet() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
}

// Update planet position based on orbit
void Planet::update(float currentTime) {
    // Update position based on orbital movement
    float angle = currentTime * orbitalSpeed;
    position.x = cosf(angle) * orbitRadius;
    position.z = sinf(angle) * orbitRadius;
}

// Draw planet
void Planet::draw(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection, 
            glm::vec3 viewPos, glm::vec3 lightPos) {

    glUseProgram(shaderProgram);
    
    // Set emission strength (for sun vs planets)
    glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), emissionStrength);
    
    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
   
    // First rotate to align rotation axis properly (90 degrees around X-axis)
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Apply axial tilt
    model = glm::rotate(model, glm::radians(axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Apply self-rotation
    float rotation = glfwGetTime() * rotationSpeed;
    model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Set lighting uniforms
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.1f);
    
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    
    // Draw planet
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// And implement it
void Planet::drawOutline(GLuint shaderProgram) {
    // Bind VAO but override color/texture with solid white
    glBindVertexArray(VAO);
    
    // Set outline color to white
    glUniform3f(glGetUniformLocation(shaderProgram, "outlineColor"), 1.0f, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 1); // Add this uniform to your shader
    
    // Draw the outline
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    
    // Reset outline flag
    glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 0);
    
    glBindVertexArray(0);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Request OpenGL 3.3 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Sun Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set the viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO();

    // Load a custom font
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("Orbitron-VariableFont_wght.ttf", 32.0f);
    if (!customFont) {
        std::cout << "Failed to load custom font!" << std::endl;
        // Fall back to default font
        customFont = io.Fonts->AddFontDefault();
    }
    // Build font atlas
    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);     // Only render front-facing triangles
    glFrontFace(GL_CCW);
    glEnable(GL_STENCIL_TEST);  

    // Load shaders
    GLuint shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");
    GLuint blurProgram = createShaderProgram("finalComposite.vert", "blur.frag");
    GLuint finalCompositeProgram = createShaderProgram("finalComposite.vert", "finalComposite.frag");
    GLuint skyboxShader = createShaderProgram("skybox.vert", "skybox.frag");

    // Setup skybox
    setupSkybox();

    // Load cubemap textures for skybox
    std::vector<std::string> faces {
        "stars/right.png",
        "stars/left.png",
        "stars/top.png",
        "stars/bottom.png",
        "stars/front.png",
        "stars/back.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    // Load planet texture
    GLuint sunTexture = loadTexture("sun.jpg");
    GLuint mercuryTexture = loadTexture("mercury.jpg");
    GLuint venusTexture = loadTexture("venus.jpg");
    GLuint earthTexture = loadTexture("earth.jpg");
    GLuint marsTexture = loadTexture("mars.jpg");
    GLuint jupiterTexture = loadTexture("jupiter.jpg");
    GLuint saturnTexture = loadTexture("saturn.jpg");
    GLuint uranusTexture = loadTexture("uranus.jpg");
    GLuint neptuneTexture = loadTexture("neptune.jpg");
    pauseTexture = loadTexture("pause-solid.png");
    playTexture = loadTexture("play-solid.png");
    forwardTexture = loadTexture("forward-solid.png");

    // Create planets
    planets.push_back(new Planet(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.5f, sunTexture));
    planets.push_back(new Planet(0.1f, 2.0f, 0.3f, 4.77f, 0.0f, 0.0f, mercuryTexture));
    planets.push_back(new Planet(0.19f, 2.75f, 0.5f, 1.87f, 0.0f, 0.0f, venusTexture));
    planets.push_back(new Planet(0.2f, 3.0f, 0.5f, 1.15f, 0.0f, 0.0f, earthTexture));
    planets.push_back(new Planet(0.15f, 3.8f, 0.4f, 0.61f, 0.0f, 0.0f, marsTexture));
    planets.push_back(new Planet(0.7f, 7.0f, 0.6f, 0.097f, 0.0f, 0.0f, jupiterTexture));
    planets.push_back(new Planet(0.63f, 10.0f, 0.7f, 0.039f, 0.0f, 0.0f, saturnTexture));
    planets.push_back(new Planet(0.33f, 17.0f, 0.8f, 0.0137f, 0.0f, 0.0f, uranusTexture));
    planets.push_back(new Planet(0.3f, 30.0f, 0.9f, 0.007f, 0.0f, 0.0f, neptuneTexture));


    glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
    recreateFramebuffers(SCR_WIDTH, SCR_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;  

        processInput(window);

        // Set uniforms (same as before)
        glm::mat4 model = glm::mat4(1.0f);
        // Calculate camera position using spherical coordinates
        // Get orbit center (sun or focused planet)
        // Replace the current orbit center calculation in your main loop
        // Get orbit center (sun or focused planet)
        glm::vec3 orbitCenter(0.0f);
        
        // Update target position if following a planet
        if (followingPlanet && focusedPlanetIndex >= 0 && focusedPlanetIndex < planets.size()) {
            targetOrbitCenter = planets[focusedPlanetIndex]->position;
        }

        // Smooth interpolation between current and target positions
        if (glm::distance(currentOrbitCenter, targetOrbitCenter) > 0.01f) {
            // Non-linear easing function (exponential approach)
            float t = 1.0f - exp(-transitionSpeed * deltaTime);
            currentOrbitCenter = currentOrbitCenter + t * (targetOrbitCenter - currentOrbitCenter);
            inTransition = true;
        } else {
            inTransition = false;
        }

        // Use the smoothed position for camera calculations
        orbitCenter = currentOrbitCenter;

        // Calculate camera position using spherical coordinates around the orbit center
        float camX = orbitCenter.x + sin(glm::radians(yaw)) * cos(glm::radians(pitch)) * cameraDistance;
        float camY = orbitCenter.y + sin(glm::radians(pitch)) * cameraDistance;
        float camZ = orbitCenter.z + cos(glm::radians(yaw)) * cos(glm::radians(pitch)) * cameraDistance;

        // Create view matrix looking at the orbit center
        glm::mat4 view = glm::lookAt(
            glm::vec3(camX, camY, camZ),    // Camera position
            orbitCenter,                    // Look target (planet or sun)
            glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                      (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                      0.1f, 1000.0f);
        currentView = view;
        currentProjection = projection;

        // PASS 1A: Draw skybox to its own framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, skyboxFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Draw skybox
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxShader);
        glm::mat4 skyView = glm::mat4(glm::mat3(view)); // Remove translation
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);


        // PASS 1B: Draw sun to HDR framebuffer with bloom extraction
        glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Alpha = 0.0 indicates "no object"
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // You MUST reset this every time after binding a different framebuffer
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments); // This is critical!

        // Then draw the sun...
        glUseProgram(shaderProgram);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), camX, camY, camZ);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.1f);
        glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), 1.5f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTexture);  // Bind the actual sun texture
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

        // Update and draw all planets
        float currentTime = glfwGetTime();
        glm::vec3 lightPos(0.0f, 0.0f, 0.0f); // Sun position at origin

        // First, update which planet is hovered BEFORE any drawing
        updateHoveredPlanet(window, view, projection);

        // Reset stencil buffer
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);
        
        // Draw planets with stencil writing only for the hovered planet
        for (size_t i = 0; i < planets.size(); i++) {
            if (!isPaused) {
                float scaledTime = currentTime * timeScale;
                planets[i]->update(scaledTime);
            }
            // Special handling for hovered planet
            if (i == hoveredPlanetIndex) {
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                glStencilMask(0xFF);
            } else {
                glStencilMask(0x00); // Don't write to stencil buffer
            }
            
            planets[i]->draw(shaderProgram, view, projection, 
                        glm::vec3(camX, camY, camZ), lightPos);
        }

        // In your hover rendering code:
        if (hoveredPlanetIndex >= 0 && hoveredPlanetIndex < planets.size()) {
            // Get planet info
            glm::vec3 planetPos = planets[hoveredPlanetIndex]->position;
            float planetRadius = planets[hoveredPlanetIndex]->radius;
            
            // Enable blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Disable depth testing to make outline visible through planet
            glDisable(GL_DEPTH_TEST);
            
            // Draw outline with ring effect
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, planetPos);
            model = glm::rotate(model, glm::radians(planets[hoveredPlanetIndex]->axialTilt), 
                            glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, static_cast<float>(glfwGetTime()) * planets[hoveredPlanetIndex]->rotationSpeed, 
                            glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Scale slightly larger than planet
            float outlineScale = 1.3f;
            model = glm::scale(model, glm::vec3(outlineScale));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Set ring parameters - adjust these values to control the ring appearance
            glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 1);
            glUniform3f(glGetUniformLocation(shaderProgram, "outlineColor"), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "outlineAlpha"), 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "innerRadius"), 0.3f); // Controls gap size
            glUniform1f(glGetUniformLocation(shaderProgram, "outerRadius"), 0.0f); // Edge of sphere
            
            // Draw outline
            planets[hoveredPlanetIndex]->drawOutline(shaderProgram);
            
            // Reset state
            glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 0);
            glEnable(GL_DEPTH_TEST);
        }

        // SECOND PASS: Blur the bloom texture using ping-pong
        bool horizontal = true, first_iteration = true;
        int amount = 10;
        glUseProgram(blurProgram);
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glClear(GL_COLOR_BUFFER_BIT);  // Clear each ping-pong buffer
            glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal);
            
            // Bind appropriate texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? bloomTexture : pingpongBuffer[!horizontal]);
            glUniform1i(glGetUniformLocation(blurProgram, "image"), 0);
            
            renderQuad();
            
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }


        // THIRD PASS: Final render to the screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(finalCompositeProgram);

        // Bind all three textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);
        glUniform1i(glGetUniformLocation(finalCompositeProgram, "skyboxTex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, postProcessingTexture);
        glUniform1i(glGetUniformLocation(finalCompositeProgram, "sunTex"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
        glUniform1i(glGetUniformLocation(finalCompositeProgram, "bloomTex"), 2);

        // Render final composite quad
        renderQuad();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        float windowWidth = 200.0f; // Fixed window width
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.25f - windowWidth*0.5f, 15.0f));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0)); // Auto-height
        ImGui::SetNextWindowBgAlpha(0.3f);

        ImGuiStyle& style = ImGui::GetStyle();
        float oldRounding = style.WindowRounding;
        style.WindowRounding = 5.0f;
        ImGui::Begin("Planet Info", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        // Begin using custom font
        ImGui::PushFont(customFont);

        // Get the text content
        std::string planetText = getPlanetName(focusedPlanetIndex);

        // Calculate text size
        ImVec2 textSize = ImGui::CalcTextSize(planetText.c_str());

        // Calculate position to center text in window
        float textPosX = (windowWidth - textSize.x) * 0.5f;

        // Add spacing on the left to center the text
        ImGui::SetCursorPosX(textPosX);

        // Draw the text
        ImGui::Text("%s", planetText.c_str());

        // Return to default font
        ImGui::PopFont();

        ImGui::End();
            
        // Remove the current speed controls window code and replace with this:

        // 1. Play/Pause Button Window
        float buttonSize = 40.0f;  // Button size
        float windowPadding = 15.0f;  // Padding around button
        float windowSpacing = 0.0f;  // Space between windows

        float imageSize = 15.0f;
        int framePadding = 15;

        // 1. Play/Pause Button Window - keep window completely transparent
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.5 - buttonSize - windowPadding*2 - windowSpacing - buttonSize - windowPadding*2 - 10, 0));
        ImGui::SetNextWindowSize(ImVec2(buttonSize + windowPadding*2, buttonSize + windowPadding*2));
        ImGui::SetNextWindowBgAlpha(0.0f); // Keep window transparent

        // Remove window styling but keep variables for button
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding, windowPadding));

        // Apply styling to the buttons - add border
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f); // Round button corners
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); // Add border to button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Semi-transparent background
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.4f)); // Slightly lighter on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f)); // Even lighter when clicked
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.3f)); // White border
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));

        ImGui::Begin("Play/Pause", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoBackground);

       // For Play/Pause button:
        if (ImGui::ImageButton(isPaused ? "PlayButton" : "PauseButton", 
            (ImTextureID)(uintptr_t)(isPaused ? playTexture : pauseTexture), 
            ImVec2(imageSize, imageSize),
            ImVec2(0, 0), ImVec2(1, 1),
            ImVec4(0,0,0,0))) {  // Use integer padding value
            isPaused = !isPaused;
            timeScale = isPaused ? 0.0f : (timeScale >= 2.0f ? 2.0f : 1.0f);
        }


        ImGui::End();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(5); 

        // 2. Speed Control Button Window - similar changes
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.5 - buttonSize - windowPadding*2 - 10, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(buttonSize + windowPadding*2, buttonSize + windowPadding*2));
        ImGui::SetNextWindowBgAlpha(0.0f); // Make window fully transparent

        // Remove window styling but keep variables for button
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding, windowPadding));

        // Apply styling to the buttons instead
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f); // Round button corners
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); // Add border to button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Semi-transparent background
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.4f)); // Slightly lighter on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f)); // Even lighter when clicked
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.3f)); // White border
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));

        ImGui::Begin("Speed Control", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoBackground); // Add NoBackground flag

        bool isDoubleSpeed = (timeScale >= 2.0f);
        // 4. Update Speed button similarly
        if (ImGui::ImageButton("SpeedButton", 
            (ImTextureID)(uintptr_t)forwardTexture,
            ImVec2(imageSize, imageSize),  // Use smaller image size 
            ImVec2(0,0), ImVec2(1,1),
            ImVec4(0,0,0,0))) {  // Add padding
            isDoubleSpeed = !isDoubleSpeed;
            if (!isPaused) {
                timeScale = isDoubleSpeed ? 2.0f : 1.0f;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(5);

        style.WindowRounding = oldRounding;
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Swap buffers
        glfwSwapBuffers(window);
        
        // Poll for and process events
        glfwPollEvents();

    }

    // Clean up and exit
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &sunTexture);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &cubemapTexture);
    for (Planet* planet : planets) {
        delete planet;
    }
    planets.clear();
    glfwDestroyWindow(window);
    glfwTerminate();

    // 5. Add shutdown code before program termination
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

GLuint loadShader(const char* path, GLenum shaderType) {
    std::ifstream shaderFile(path);
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    std::string shaderCode = shaderStream.str();
    const char* shaderSource = shaderCode.c_str();

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    GLuint vertexShader = loadShader(vertexPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = loadShader(fragmentPath, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust zoom speed with this multiplier
    float zoomSpeed = 0.5f;
    
    // Update camera distance based on scroll
    cameraDistance -= (float)yoffset * zoomSpeed;
    
    // Clamp distance to prevent getting too close or too far
    if (cameraDistance < 3.0f)
        cameraDistance = 3.0f;  // Minimum distance
    if (cameraDistance > 100.0f)
        cameraDistance = 100.0f;  // Maximum distance
}

// Update your mouse_button_callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    static glm::mat4 lastView;
    static glm::mat4 lastProjection;
    
    // Store view and projection matrices for click detection
    extern glm::mat4 currentView;
    extern glm::mat4 currentProjection;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Check if we're hovering over a planet
            if (hoveredPlanetIndex >= 0) {
                // Focus on the hovered planet
                focusedPlanetIndex = hoveredPlanetIndex;
                followingPlanet = true;
                targetOrbitCenter = planets[hoveredPlanetIndex]->position;
                inTransition = true;
                std::cout << "Now focusing on planet " << hoveredPlanetIndex << std::endl;
            } else {
                // Not hovering over a planet, enable orbit mode
                orbitActive = true;
                firstMouse = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        } else if (action == GLFW_RELEASE) {
            orbitActive = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
    // Reset focus to sun with right click
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        focusedPlanetIndex = -1;
        followingPlanet = false;
        targetOrbitCenter = glm::vec3(0.0f); // Set target to sun
        inTransition = true;
        std::cout << "Now focusing on sun" << std::endl;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!orbitActive) {
        return;
    }
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }
    
    // Calculate offset since last frame
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed: y ranges bottom to top
    lastX = xpos;
    lastY = ypos;
    
    // Apply sensitivity for smoother control
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    // Update angles
    yaw += xoffset;
    pitch += yoffset;
    
    // Constrain pitch to prevent flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

