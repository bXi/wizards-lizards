#version 450

// Outline glow effect - creates a halo around the sprite

layout(set = 2, binding = 0) uniform sampler2D input_texture;

layout(location = 0) in vec2 tex_coords;
layout(location = 0) out vec4 out_color;

// Effect parameters
layout(set = 3, binding = 0) uniform EffectParams {
    float intensity;      // Glow intensity/size
    vec3 glowColor;       // Glow color (RGB)
    float time;           // For animated effects
} params;

void main() {
    // Sample the original sprite
    vec4 sprite = texture(input_texture, tex_coords);
    
    // Calculate texture size for pixel offsets
    vec2 texelSize = 1.0 / textureSize(input_texture, 0);
    
    // Sample surrounding pixels to detect edges
    float glowAmount = 0.0;
    float sampleRadius = params.intensity * 3.0; // How far to search for edges
    
    // Only add glow if current pixel is mostly transparent
    if (sprite.a < 0.1) {
        // Sample in multiple directions to find sprite edges
        for (float angle = 0.0; angle < 6.28318; angle += 0.785398) { // 8 directions
            for (float dist = 1.0; dist <= sampleRadius; dist += 1.0) {
                vec2 offset = vec2(cos(angle), sin(angle)) * dist * texelSize;
                float sampleAlpha = texture(input_texture, tex_coords + offset).a;
                
                // If we found the sprite nearby, add glow
                if (sampleAlpha > 0.1) {
                    // Glow fades with distance
                    float falloff = 1.0 - (dist / sampleRadius);
                    glowAmount += sampleAlpha * falloff * 0.3; // Increased multiplier
                }
            }
        }
    }
    
    // Normalize glow amount - much brighter now
    glowAmount = min(glowAmount, 1.0);
    
    // Combine sprite with glow
    vec4 glow = vec4(params.glowColor * glowAmount, glowAmount);
    
    // Composite: glow behind the sprite
    out_color = mix(glow, sprite, sprite.a);
}
