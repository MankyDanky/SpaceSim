#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float emissionStrength;

// Add these uniforms for outline rendering
uniform bool isOutline = false;
uniform vec3 outlineColor = vec3(1.0, 1.0, 1.0);
uniform float outlineAlpha = 1.0;
uniform float innerRadius = 0.95; // Controls inner boundary of the outline ring
uniform float outerRadius = 1.0;  // Controls outer boundary of the outline ring

void main()
{
    if (isOutline) {
        // Calculate distance from fragment to center in view space
        vec3 viewNormal = normalize(Normal);
        
        // For a sphere, the normal is equivalent to position in object space
        // Use this to calculate where we are on the sphere surface
        float ndotv = abs(dot(viewNormal, normalize(viewPos - FragPos)));
        
        // Discard fragments that aren't within our ring boundaries
        // This creates a ring effect at the edges of the sphere
        if (ndotv > innerRadius) {
            // Inside the gap - make transparent
            discard;
        }
        
        // For the remaining fragments, render the outline
        FragColor = vec4(outlineColor, outlineAlpha);
        BloomColor = vec4(0, 0, 0, outlineAlpha);
        return;
    }

    // Regular rendering below (unchanged)
    // ambient
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float shininess = 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // emission
    vec3 emission = texture(texture1, TexCoords).rgb * emissionStrength;

    // final color
    vec3 result = (ambient + diffuse + specular + emission) * texture(texture1, TexCoords).rgb;
    FragColor = vec4(result, 1.0f);

    float brightness = dot(FragColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
    if (emissionStrength > 0)
        BloomColor = vec4(FragColor.rgb * emissionStrength, 1.0f);
    else
        BloomColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}