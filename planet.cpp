#include "planet.h"
#include "objects.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Planet::Planet(std::string name,float radius, float orbitRadius, float rotationSpeed, float orbitalSpeed, 
    float axialTilt, float emissionStrength, GLuint texture,
    bool hasRings, float ringInnerRadius, float ringOuterRadius, GLuint ringTexture)
    : name(name), radius(radius), orbitRadius(orbitRadius), rotationSpeed(rotationSpeed),
    orbitalSpeed(orbitalSpeed), axialTilt(axialTilt), emissionStrength(emissionStrength),
    position(glm::vec3(orbitRadius, 0.0f, 0.0f)), texture(texture),
    hasRings(hasRings), ringInnerRadius(ringInnerRadius), ringOuterRadius(ringOuterRadius), 
    ringTexture(ringTexture) {
        
    rotation = 0.0f;
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
    
    // If this planet has rings, create the ring geometry
    if (hasRings) {
        std::vector<float> ringVertices;
        std::vector<unsigned int> ringIndices;
        createRing(ringInnerRadius, ringOuterRadius, 100, ringVertices, ringIndices);
        ringIndexCount = ringIndices.size();
        
        // Create VAO, VBO, and EBO for the ring
        glGenVertexArrays(1, &ringVAO);
        glGenBuffers(1, &ringVBO);
        glGenBuffers(1, &ringEBO);
        
        glBindVertexArray(ringVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
        glBufferData(GL_ARRAY_BUFFER, ringVertices.size() * sizeof(float), 
                    &ringVertices[0], GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ringIndices.size() * sizeof(unsigned int), 
                    &ringIndices[0], GL_STATIC_DRAW);
        
        // Vertex positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Vertex normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Vertex texture coords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        std::cout << "Created rings for planet: " << ringIndexCount << " indices" << std::endl;
    }

    glBindVertexArray(0);
    
    std::cout << "Planet created: VAO=" << VAO << ", texture=" << texture << ", indices=" << indexCount << std::endl;
}

Planet::~Planet() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    if (hasRings) {
        glDeleteVertexArrays(1, &ringVAO);
        glDeleteBuffers(1, &ringVBO);
        glDeleteBuffers(1, &ringEBO);
    }
}

void Planet::update(float currentTime) {
    // Update position based on orbital movement
    float angle = currentTime * orbitalSpeed;
    position.x = cosf(angle) * orbitRadius;
    position.z = sinf(angle) * orbitRadius;
    rotation = currentTime * rotationSpeed;
}

void Planet::draw(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection, 
    glm::vec3 viewPos, glm::vec3 lightPos) {

    glUseProgram(shaderProgram);

    // Set emission strength
    glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), emissionStrength);

    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // First rotate to align rotation axis properly (90 degrees around X-axis)
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Apply axial tilt
    model = glm::rotate(model, glm::radians(axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));

    // Apply self-rotation
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

    if (hasRings) {
    // Create ring model matrix
    glm::mat4 ringModel = glm::mat4(1.0f);
    ringModel = glm::translate(ringModel, position);
    ringModel = glm::rotate(ringModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Apply proper orientation for rings
    ringModel = glm::rotate(ringModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ringModel = glm::rotate(ringModel, glm::radians(axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));

    // Set ring model matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(ringModel));

    // Tell shader this is a ring (if you want special handling)
    glUniform1i(glGetUniformLocation(shaderProgram, "isRing"), 1);

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable face culling for double-sided rings
    glDisable(GL_CULL_FACE);

    // Bind ring texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ringTexture);

    // Draw rings as triangle strip
    glBindVertexArray(ringVAO);
    glDrawElements(GL_TRIANGLE_STRIP, ringIndexCount, GL_UNSIGNED_INT, 0);

    // Reset states
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glUniform1i(glGetUniformLocation(shaderProgram, "isRing"), 0);
    }

    glBindVertexArray(0);
}

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