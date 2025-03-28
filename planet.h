#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Planet {
    public:
        std::string name;
        GLuint VAO, VBO, EBO;
        GLuint texture;
        float radius;
        float orbitRadius;
        float rotationSpeed;
        float rotation;
        float orbitalSpeed;
        float axialTilt;
        float emissionStrength;
        glm::vec3 position;
        unsigned int indexCount;
       
        // Ring-related variables
        bool hasRings;
        GLuint ringVAO, ringVBO, ringEBO;
        GLuint ringTexture;
        float ringInnerRadius;
        float ringOuterRadius;
        unsigned int ringIndexCount;

         // Constructor
         Planet(std::string name, float radius, float orbitRadius, float rotationSpeed, float orbitalSpeed, 
            float axialTilt, float emissionStrength, GLuint texture,
            bool hasRings = false, float ringInnerRadius = 0.0f, 
            float ringOuterRadius = 0.0f, GLuint ringTexture = 0);
        
        // Destructor
        ~Planet();
        
        // Update planet position based on orbit
        void update(float currentTime);
        
        // Draw planet
        void draw(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection, 
                  glm::vec3 viewPos, glm::vec3 lightPos);
        
        void drawOutline(GLuint shaderProgram);
    };