#include <vector>

void createSphere(float radius, int sectorCount, int stackCount, std::vector<float>& vertices, std::vector<unsigned int>& indices);

void createRing(float innerRadius, float outerRadius, int segments, std::vector<float>& vertices, std::vector<unsigned int>& indices);

void createSkybox(unsigned int &skyboxVAO, unsigned int &skyboxVBO);

void renderQuad(unsigned int &quadVAO, unsigned int &quadVBO);