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

    // final color
    vec3 result = (ambient + diffuse + specular + emission) * texture(texture1, TexCoords).rgb;
    FragColor = texture(texture1, TexCoords);

    float brightness = dot(FragColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f));
    if (brightness > 0.05f)
        BloomColor = vec4(FragColor.rgb, 1.0f);
    else
        BloomColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}