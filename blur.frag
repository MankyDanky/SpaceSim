#version 330 core
out vec4 FragColor;
  
in vec2 texCoords;

uniform sampler2D image;
uniform bool horizontal;
float spreadBlur = 5.0f;
float bloomIntensity = 1.2;

// How far from the center to take samples
const int MAX_RADIUS = 20;

void main()
{             
    // Calculate effective radius based on spread (min 5, max MAX_RADIUS)
    int radius = min(max(int(ceil(spreadBlur * 3.0)), 5), MAX_RADIUS);
    
    // Calculate the weights using the Gaussian equation
    float weights[MAX_RADIUS];
    float weightSum = 0.0;
    
    // Calculate initial weight (center pixel)
    weights[0] = 1.0;
    weightSum += weights[0];
    
    // Calculate remaining weights
    for (int i = 1; i < radius; i++) {
        float x = float(i) / spreadBlur;
        weights[i] = exp(-0.5 * x * x);
        weightSum += weights[i] * 2.0; // Multiplied by 2 for left and right samples
    }
    
    // Normalize weights so they sum to 1.0
    for (int i = 0; i < radius; i++) {
        weights[i] /= weightSum;
    }

    // Get texel size
    vec2 tex_offset = 1.0f / textureSize(image, 0);
    
    // Sample center pixel
    vec3 result = texture(image, texCoords).rgb * weights[0];

    // Calculate horizontal blur
    if(horizontal) {
        for(int i = 1; i < radius; i++) {
            // Scale offset by i for wider sampling as i increases
            float offset = float(i) * tex_offset.x;
            
            // Take into account pixels to the right
            result += texture(image, texCoords + vec2(offset, 0.0)).rgb * weights[i];
            // Take into account pixels on the left
            result += texture(image, texCoords - vec2(offset, 0.0)).rgb * weights[i];
        }
    }
    // Calculate vertical blur
    else {
        for(int i = 1; i < radius; i++) {
            float offset = float(i) * tex_offset.y;
            
            // Take into account pixels above
            result += texture(image, texCoords + vec2(0.0, offset)).rgb * weights[i];
            // Take into account pixels below
            result += texture(image, texCoords - vec2(0.0, offset)).rgb * weights[i];
        }
    }
    
    // Output final color
    result *= bloomIntensity;
    FragColor = vec4(result, 1.0f);
}