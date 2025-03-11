#version 330 core
out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D skyboxTex;
uniform sampler2D sunTex;
uniform sampler2D bloomTex;
uniform float gamma = 2.2;

// In finalComposite.frag
void main() {
    // Sample from all textures
    vec3 skybox = texture(skyboxTex, texCoords).rgb;
    vec3 sun = texture(sunTex, texCoords).rgb;  
    vec3 bloom = texture(bloomTex, texCoords).rgb;
    
    // Calculate sun intensity
    float sunIntensity = dot(sun + bloom, vec3(0.2126, 0.7152, 0.0722));
    
    // Improved blending between skybox and sun+bloom
    float threshold = 0.2; // Adjust this value to control the transition
    float blend = smoothstep(0.0, threshold, sunIntensity);

    // Blend between skybox and sun+bloom based on intensity
    vec3 result = mix(skybox + bloom, sun + bloom, blend);
    
    // Apply tone mapping
    float exposure = 0.8f;
    vec3 toneMapped = vec3(1.0f) - exp(-result * exposure);
    
   FragColor = vec4(pow(toneMapped, vec3(1.0f / gamma)), 1.0);
}