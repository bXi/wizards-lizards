#version 450

layout(set = 2, binding = 0) uniform sampler2D input_texture;

layout(location = 0) in vec2 tex_coords;
layout(location = 0) out vec4 out_color;

layout(set = 3, binding = 0) uniform EffectParams {
    float hueShift;   // 0.0 = no shift, 1.0 = full 360 degree rotation
} params;

// RGB -> HSV
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV -> RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 sprite = texture(input_texture, tex_coords);

    if (sprite.a < 0.01) {
        out_color = sprite;
        return;
    }

    vec3 hsv = rgb2hsv(sprite.rgb);

    // Only shift pixels that are in the blue range (hue ~0.55-0.72 in 0-1 space)
    // This keeps skin tones, whites, browns, and greys untouched
    float hue = hsv.x;
    float blueness = smoothstep(0.50, 0.58, hue) * (1.0 - smoothstep(0.67, 0.75, hue));

    hsv.x = fract(hsv.x + params.hueShift * blueness);
    vec3 shifted = hsv2rgb(hsv);

    out_color = vec4(shifted, sprite.a);
}
