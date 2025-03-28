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

// Outline uniforms
uniform bool isOutline = false;
uniform vec3 outlineColor = vec3(1.0, 1.0, 1.0);
uniform float outlineAlpha = 1.0;
uniform float innerRadius = 0.95;

void main()
{
    if (isOutline) {
        // Calculate distance from fragment to center in view space
        vec3 viewNormal = normalize(Normal);
        float ndotv = abs(dot(viewNormal, normalize(viewPos - FragPos)));
        
        // First discard fragments inside the gap (same as before)
        if (ndotv > innerRadius) {
            discard;
        }
        
        // Calculate angle around the planet for the compass effect
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 rightDir = normalize(cross(vec3(0.0, 1.0, 0.0), viewDir));
        vec3 upDir = normalize(cross(viewDir, rightDir));
        
        // Calculate 2D position on unit circle of sphere
        float x = dot(Normal, rightDir);
        float y = dot(Normal, upDir);
        
        // Calculate angle in radians and convert to degrees
        float angle = degrees(atan(y, x));
        if (angle < 0.0) angle += 360.0;
        
        // Get modulo 90 to repeat every quadrant
        float quadrantAngle = mod(angle, 90.0);
        
        // Width of cardinal direction segments (adjust as needed)
        float cardinalWidth = 30.0; 
        
        // Discard diagonal segments
        if (quadrantAngle > cardinalWidth && quadrantAngle < (90.0 - cardinalWidth)) {
            discard;
        }
        
        // For the remaining fragments, render the outline
        FragColor = vec4(outlineColor, outlineAlpha);
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0); 
        return;
    }

    // Regular rendering below
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float shininess = 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Emission
    vec3 emission = texture(texture1, TexCoords).rgb * emissionStrength;

    // Final color
    vec3 result = (ambient + diffuse + specular + emission) * texture(texture1, TexCoords).rgb;
    FragColor = vec4(result, 1.0f);

    float brightness = dot(FragColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
    if (emissionStrength > 0)
        BloomColor = vec4(FragColor.rgb * emissionStrength, 1.0f);
    else
        BloomColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}