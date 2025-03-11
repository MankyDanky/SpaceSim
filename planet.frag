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

void main()
{
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

    // Store linearized depth in alpha
    float near = 0.1;
    float far = 1000.0;
    float linearDepth = (2.0 * near) / (far + near - gl_FragCoord.z * (far - near));

    // final color
    vec3 result = (ambient + diffuse + specular + emission) * texture(texture1, TexCoords).rgb;
    FragColor = vec4(result, linearDepth);
}