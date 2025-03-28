#version 330 core
out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D skyboxTex;
uniform sampler2D sunTex;
uniform sampler2D bloomTex;
uniform float gamma = 2.2;

void main() {
    vec3 skybox = texture(skyboxTex, texCoords).rgb;
    vec4 scene = texture(sunTex, texCoords);
    vec3 bloom = texture(bloomTex, texCoords).rgb;
    
    // Check for object brightness 
    float objectBrightness = dot(scene.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    vec3 result;
    if (scene.a > 0.01 && objectBrightness > 0.01) {
        // Object is present AND visible
        result = scene.rgb + bloom;
    } else {
        // No visible object here, show skybox
        result = skybox + bloom;
    }
    
    // Apply tone mapping
    float exposure = 0.8;
    vec3 toneMapped = vec3(1.0) - exp(-result * exposure);
    
    // Apply gamma correction
    FragColor = vec4(pow(toneMapped, vec3(1.0/gamma)), 1.0);
}