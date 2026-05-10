#version 450

// HRC final composite: emissive scene + blurred fluence → display.
// Fluence texture is already accumulated from all 4 directions and blurred.

layout(set = 2, binding = 0) uniform sampler2D uScene;     // hrc_scene (nearest)
layout(set = 2, binding = 1) uniform sampler2D uFluence;   // hrc_fluence (linear, probe res)

layout(set = 3, binding = 0) uniform BlitUniforms {
    float uExposure;
    float uTexW;
    float uTexH;
};

layout(location = 0) in  vec2 tex_coords;
layout(location = 0) out vec4 FragColor;

#define TWO_PI 6.28318530718

vec3 acesTonemap(vec3 x) {
    return clamp(
        (x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0
    );
}

vec3 linearToSrgb(vec3 c) {
    return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2));
}

uint pcg(uint v) {
    uint s = v * 747796405u + 2891336453u;
    uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (w >> 22u) ^ w;
}

vec3 triangularDither() {
    uvec2 fc = uvec2(gl_FragCoord.xy);
    uint seed = fc.x + fc.y * 8192u;
    float r0 = float(pcg(seed))      / 4294967295.0;
    float r1 = float(pcg(seed + 1u)) / 4294967295.0;
    return vec3((r0 + r1 - 1.0) / 255.0);
}

void main() {
    vec2 uv    = vec2(gl_FragCoord.x / uTexW, gl_FragCoord.y / uTexH);
    vec4 scene = texelFetch(uScene, ivec2(gl_FragCoord.xy), 0);
    vec3 fluence = texture(uFluence, uv).rgb;

    vec3 emissive = scene.rgb * scene.a;
    vec3 indirect = (fluence / TWO_PI) * (1.0 - scene.a);
    vec3 hdr      = (emissive + indirect) * uExposure;

    vec3 out_rgb = linearToSrgb(acesTonemap(hdr)) + triangularDither();
    FragColor = vec4(out_rgb, 1.0);
}
