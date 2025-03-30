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
#include "vendor/stb/stb_image.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_glfw.h"
#include "vendor/imgui/backends/imgui_impl_opengl3.h"
#include "planet.h"
#include "texture.h"
#include "objects.h"
#include "shader.h"

int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

int focusedPlanetIndex = 0;
int hoveredPlanetIndex = -1; 
bool followingPlanet = false;

glm::mat4 currentView;
glm::mat4 currentProjection;

// Simulation speed control
float simulationTime = 0.0f;
float timeScale = 1.0f;
bool isPaused = false;  
GLuint pauseTexture, playTexture, forwardTexture;

// Camera settings
glm::vec3 currentOrbitCenter(0.0f); 
glm::vec3 targetOrbitCenter(0.0f);
float transitionSpeed = 4.0f;
bool inTransition = false;
float cameraDistance = 10.0f;
float lastX = 400, lastY = 300;
float yaw = 0.0f;
float pitch = 0.0f;
bool firstMouse = true;
bool orbitActive = false; 

std::vector<Planet*> planets;

unsigned int postProcessingFBO, rbo;
unsigned int postProcessingTexture, bloomTexture;
unsigned int skyboxFBO, skyboxTexture;
unsigned int pingpongFBO[2], pingpongBuffer[2];

float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// Callback functions
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
    
    // Recreate framebuffers with new size
    recreateFramebuffers(width, height);
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
int findClickedPlanet(GLFWwindow* window, const glm::mat4& view, const glm::mat4& projection, const std::vector<Planet*>& planets) {
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Space Simulation", nullptr, nullptr);
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
    ImFont* customFont = io.Fonts->AddFontFromFileTTF("fonts/Orbitron-VariableFont_wght.ttf", 32.0f);
    if (!customFont) {
        std::cout << "Failed to load custom font!" << std::endl;
        // Fall back to default font
        customFont = io.Fonts->AddFontDefault();
    }
    // Build font atlas
    ImGui_ImplOpenGL3_CreateFontsTexture();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);     // Only render front-facing triangles
    glFrontFace(GL_CCW);
    glEnable(GL_STENCIL_TEST);  

    // Load shaders
    GLuint shaderProgram = createShaderProgram("shaders/main.vert", "shaders/main.frag");
    GLuint blurProgram = createShaderProgram("shaders/finalComposite.vert", "shaders/blur.frag");
    GLuint finalCompositeProgram = createShaderProgram("shaders/finalComposite.vert", "shaders/finalComposite.frag");
    GLuint skyboxShader = createShaderProgram("shaders/skybox.vert", "shaders/skybox.frag");

    // Setup skybox
    unsigned int skyboxVAO = 0, skyboxVBO = 0;
    createSkybox(skyboxVAO, skyboxVBO);

    // Create VAO and VBO for quad
    unsigned int quadVAO = 0, quadVBO = 0;

    // Load cubemap textures for skybox
    std::vector<std::string> faces {
        "textures/right.png",
        "textures/left.png",
        "textures/top.png",
        "textures/bottom.png",
        "textures/front.png",
        "textures/back.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    // Load planet texture
    GLuint sunTexture = loadTexture("textures/sun.jpg");
    GLuint mercuryTexture = loadTexture("textures/mercury.jpg");
    GLuint venusTexture = loadTexture("textures/venus.jpg");
    GLuint earthTexture = loadTexture("textures/earth.jpg");
    GLuint marsTexture = loadTexture("textures/mars.jpg");
    GLuint jupiterTexture = loadTexture("textures/jupiter.jpg");
    GLuint saturnTexture = loadTexture("textures/saturn.jpg");
    GLuint uranusTexture = loadTexture("textures/uranus.jpg");
    GLuint neptuneTexture = loadTexture("textures/neptune.jpg");
    GLuint saturnRingTexture = loadTexture("textures/saturn_ring.png");
    pauseTexture = loadTexture("textures/pause.png");
    playTexture = loadTexture("textures/play.png");
    forwardTexture = loadTexture("textures/forward.png");

    // Create planets
    planets.push_back(new Planet("Sun",1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.5f, sunTexture));
    planets.push_back(new Planet("Mercury",0.1f, 2.0f, 0.3f, 4.77f, 0.0f, 0.0f, mercuryTexture));
    planets.push_back(new Planet("Venus",0.19f, 2.75f, 0.5f, 1.87f, 0.0f, 0.0f, venusTexture));
    planets.push_back(new Planet("Earth",0.2f, 3.35f, 0.5f, 1.15f, 0.0f, 0.0f, earthTexture));
    planets.push_back(new Planet("Mars", 0.15f, 4.5f, 0.4f, 0.61f, 0.0f, 0.0f, marsTexture));
    planets.push_back(new Planet("Jupiter", 0.7f, 7.0f, 0.6f, 0.097f, 0.0f, 0.0f, jupiterTexture));
    planets.push_back(new Planet("Saturn", 0.63f, 10.0f, 0.7f, 0.039f, 0.0f, 0.0f, saturnTexture,
    true, 0.7f, 1.2f, saturnRingTexture));
    planets.push_back(new Planet("Uranus", 0.33f, 17.0f, 0.8f, 0.0137f, 0.0f, 0.0f, uranusTexture));
    planets.push_back(new Planet("Neptune", 0.3f, 30.0f, 0.9f, 0.007f, 0.0f, 0.0f, neptuneTexture));


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

        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 orbitCenter(0.0f);
        
        // Update target position if following a planet
        if (followingPlanet && focusedPlanetIndex >= 0 && focusedPlanetIndex < planets.size()) {
            targetOrbitCenter = planets[focusedPlanetIndex]->position;
        }

        // Smooth interpolation between current and target positions
        if (glm::distance(currentOrbitCenter, targetOrbitCenter) > 0.01f) {
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
            glm::vec3(camX, camY, camZ),
            orbitCenter,
            glm::vec3(0.0f, 1.0f, 0.0f) 
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                      (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                      0.1f, 1000.0f);
        currentView = view;
        currentProjection = projection;

        // Draw skybox to its own framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, skyboxFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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


        // Draw sun to HDR framebuffer with bloom extraction
        glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
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
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

        // Update and draw all planets
        if (!isPaused) {
            simulationTime += deltaTime * timeScale;
        }
        glm::vec3 lightPos(0.0f, 0.0f, 0.0f); 

        // Stencil buffer setup for outline
        updateHoveredPlanet(window, view, projection);
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);

        for (size_t i = 0; i < planets.size(); i++) {
            if (!isPaused) {
                planets[i]->update(simulationTime);
            }

            // Set up stencil buffer for outline
            if (i == hoveredPlanetIndex) {
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                glStencilMask(0xFF);
            } else {
                glStencilMask(0x00);
            }
            
            planets[i]->draw(shaderProgram, view, projection, 
                        glm::vec3(camX, camY, camZ), lightPos);
        }

        if (hoveredPlanetIndex >= 0 && hoveredPlanetIndex < planets.size()) {
            glm::vec3 planetPos = planets[hoveredPlanetIndex]->position;
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
            
            // Draw outline
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, planetPos);
            model = glm::rotate(model, glm::radians(planets[hoveredPlanetIndex]->axialTilt), 
                            glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::rotate(model, static_cast<float>(glfwGetTime()) * planets[hoveredPlanetIndex]->rotationSpeed, 
                            glm::vec3(0.0f, 1.0f, 0.0f));
            float outlineScale = 1.3f;
            model = glm::scale(model, glm::vec3(outlineScale));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 1);
            glUniform3f(glGetUniformLocation(shaderProgram, "outlineColor"), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "outlineAlpha"), 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "innerRadius"), 0.3f);
            planets[hoveredPlanetIndex]->drawOutline(shaderProgram);
            
            glUniform1i(glGetUniformLocation(shaderProgram, "isOutline"), 0);
            glEnable(GL_DEPTH_TEST);
        }

        // Blur the bloom texture using ping-pong
        bool horizontal = true, first_iteration = true;
        int amount = 10;
        glUseProgram(blurProgram);
        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(blurProgram, "horizontal"), horizontal);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? bloomTexture : pingpongBuffer[!horizontal]);
            glUniform1i(glGetUniformLocation(blurProgram, "image"), 0);
            
            renderQuad(quadVAO, quadVBO);
            
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }

        // Final render to the screen
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
        renderQuad(quadVAO, quadVBO);
        
        // Render ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        float windowWidth = 200.0f;
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.25f - windowWidth*0.5f, 15.0f));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0));
        ImGui::SetNextWindowBgAlpha(0.3f);

        ImGuiStyle& style = ImGui::GetStyle();
        float oldRounding = style.WindowRounding;
        style.WindowRounding = 5.0f;
        ImGui::Begin("Planet Info", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        // Begin using custom font
        ImGui::PushFont(customFont);

        // Calculate text size
        ImVec2 textSize = ImGui::CalcTextSize(planets[focusedPlanetIndex]->name.c_str());

        // Calculate position to center text in window
        float textPosX = (windowWidth - textSize.x) * 0.5f;

        // Add spacing on the left to center the text
        ImGui::SetCursorPosX(textPosX);

        // Draw the text
        ImGui::Text("%s", planets[focusedPlanetIndex]->name.c_str());

        // Return to default font
        ImGui::PopFont();

        ImGui::End();

        // Play/Pause Button Window
        float buttonSize = 40.0f;
        float windowPadding = 15.0f;
        float windowSpacing = 0.0f;

        float imageSize = 15.0f;
        int framePadding = 15;

        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.5 - buttonSize - windowPadding*2 - windowSpacing - buttonSize - windowPadding*2 - 10, 0));
        ImGui::SetNextWindowSize(ImVec2(buttonSize + windowPadding*2, buttonSize + windowPadding*2));
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding, windowPadding));

        // Apply styling to the buttons
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

       // Play/Pause button
        if (ImGui::ImageButton(isPaused ? "PlayButton" : "PauseButton", 
            (ImTextureID)(uintptr_t)(isPaused ? playTexture : pauseTexture), 
            ImVec2(imageSize, imageSize),
            ImVec2(0, 0), ImVec2(1, 1),
            ImVec4(0,0,0,0))) {
            isPaused = !isPaused;
            timeScale = isPaused ? 0.0f : (timeScale >= 2.0f ? 2.0f : 1.0f);
        }


        ImGui::End();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(5); 

        // Speed Control Button Window
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH*0.5 - buttonSize - windowPadding*2 - 10, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(buttonSize + windowPadding*2, buttonSize + windowPadding*2));
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(windowPadding, windowPadding));

        // Apply styling to the buttons
        bool isDoubleSpeed = (timeScale >= 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, (isDoubleSpeed? ImVec4(0.256f, 0.53f, 0.96f, 0.5f) : ImVec4(0.0f, 0.0f, 0.0f, 0.3f))); // Semi-transparent backgroun 0.256f, 0.53f, 0.96f       
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (isDoubleSpeed? ImVec4(0.356f, 0.63f, 0.96f, 0.6f) : ImVec4(0.1f, 0.1f, 0.1f, 0.4f))); // Slightly lighter on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (isDoubleSpeed? ImVec4(0.37f, 0.65f, 0.96f, 0.6f) : ImVec4(0.2f, 0.2f, 0.2f, 0.5f))); // Even lighter when clicked
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.3f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));

        ImGui::Begin("Speed Control", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoBackground);

        // Speed control button
        if (ImGui::ImageButton("SpeedButton", 
            (ImTextureID)(uintptr_t)forwardTexture,
            ImVec2(imageSize, imageSize),
            ImVec2(0,0), ImVec2(1,1),
            ImVec4(0,0,0,0))) { 
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
    glDeleteProgram(blurProgram);
    glDeleteProgram(finalCompositeProgram);
    glDeleteProgram(skyboxShader);
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
    glDeleteTextures(1, &postProcessingTexture);
    glDeleteTextures(1, &bloomTexture);
    glDeleteTextures(2, pingpongBuffer);
    glDeleteTextures(1, &pauseTexture);
    glDeleteTextures(1, &playTexture);
    glDeleteTextures(1, &forwardTexture);
    glDeleteFramebuffers(1, &postProcessingFBO);
    glDeleteFramebuffers(1, &skyboxFBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &sunTexture);
    glDeleteTextures(1, &mercuryTexture);
    glDeleteTextures(1, &venusTexture);
    glDeleteTextures(1, &earthTexture);
    glDeleteTextures(1, &marsTexture);
    glDeleteTextures(1, &jupiterTexture);
    glDeleteTextures(1, &saturnTexture);
    glDeleteTextures(1, &uranusTexture);
    glDeleteTextures(1, &neptuneTexture);
    glDeleteTextures(1, &saturnRingTexture);
    glfwTerminate();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    
    return 0;
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
        focusedPlanetIndex = 0;
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