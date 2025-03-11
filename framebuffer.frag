#version 330 core

out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float gamma = 2.2;

void main()
{
    vec3 fragment = texture(scene, texCoords).rgb;
    vec3 bloom = texture(bloomBlur, texCoords).rgb;
    
    // Skip completely black pixels to let skybox show through
    if (length(fragment) < 0.01 && length(bloom) < 0.01) {
        discard; // Don't write this pixel
    }
    
    vec3 color = fragment + bloom;

    float exposure = 0.8f;
    vec3 toneMapped = vec3(1.0f) - exp(-color * exposure);

    FragColor = vec4(pow(toneMapped, vec3(1.0f / gamma)), 1.0);
}