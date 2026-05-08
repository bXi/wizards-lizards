#version 450

// Holographic Radiance Cascade — Extend pass
// Builds level k of the cascade hierarchy by compositing pairs of adjacent
// level-(k-1) probes.  Each probe at level k covers 2^k pixels along the
// sweep direction and carries (2^k + 1) directional ray slots.
//
// Fragment coordinate layout:
//   x = texelX  in [0, levelProbes * numRays)
//       texelX / numRays  => probeIdx
//       texelX % numRays  => rayIdx  (which ray slot within this probe)
//   y = sliceIdx in [0, sliceCount)
//
// COMPUTE NOTE: With compute shaders this becomes a single @compute dispatch
// reading/writing storage buffers (array<vec2u> packed F16).  The 2D texture
// layout here is a 1:1 mapping of (texelX, sliceIdx) to the 1D index
//   buf[sliceIdx * rowW + texelX]
// Replace texture reads/writes with storage-buffer indexing and the rest of
// the logic stays identical.

layout(set = 2, binding = 0) uniform sampler2D uPrevLevel;
// Level 1: uPrevLevel is the scene render target (RGB=emissive, A=opacity).
// Level 2+: uPrevLevel is the previous extend RT (RGBA16F: rad.rgb, trans).

layout(set = 3, binding = 0) uniform ExtendUniforms {
    float uLevel;        // cascade level being built (1 .. N-1)
    float uProbeCount;   // number of probes for this direction (W for H dirs, H for V dirs)
    float uSliceCount;   // number of slices                    (H for H dirs, W for V dirs)
    float uSwapCoords;   // 1.0 for vertical directions (N/S): scene is read as (slice, probe)
    float uMirrorProbe;  // 1.0 for W/S directions: mirror probe index when seeding from scene
    float _pad0;
    float _pad1;
    float _pad2;
};

layout(location = 0) in  vec2 tex_coords;
layout(location = 0) out vec4 FragColor;

// Composite two ray segments (Porter-Duff "over" for transmittance/radiance).
// Paper Eq. 7:  ⟨r_n + t_n · r_f,  t_n · t_f⟩
// Stored as vec4(rad.rgb, transmittance).
vec4 compositeRay(vec4 near, vec4 far) {
    return vec4(near.rgb + far.rgb * near.a, near.a * far.a);
}

// Signed slice offset for directional ray rayIdx at a given interval half-size.
// dirToSliceOffset(i, S) = i*2 - S
// At level k: S = prevInterval = 2^(k-1).
// rayIdx=0 → -(2^(k-1))  (most negative / "downward" diagonal)
// rayIdx=S → +(2^(k-1))  (most positive / "upward" diagonal)
int dirToSliceOffset(int dirIdx, int intervalSize) {
    return dirIdx * 2 - intervalSize;
}

// Load radiance+transmittance from the previous cascade level.
// At prevLevel == 0 we read directly from the scene texture and convert to
// (rad, trans) = (scene.rgb * scene.a,  1 - scene.a).
vec4 loadPrev(int probeIdx, int rayIdx, int sliceIdx) {
    int prevLevel    = int(uLevel) - 1;
    int prevNumProbes = (int(uProbeCount) + (1 << prevLevel) - 1) >> prevLevel;
    int prevNumRays  = (prevLevel == 0) ? 1 : ((1 << prevLevel) + 1);

    // Out-of-bounds → no radiance, fully transparent (light passes through).
    if (probeIdx < 0 || probeIdx >= prevNumProbes ||
        rayIdx   < 0 || rayIdx   >= prevNumRays   ||
        sliceIdx < 0 || sliceIdx >= int(uSliceCount)) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    if (prevLevel == 0) {
        // Seed from scene texture.
        // Horizontal dirs: scene pixel is at (probeIdx, sliceIdx) = (x, y).
        // Vertical   dirs: scene pixel is at (sliceIdx, probeIdx) = (x, y).
        // W/S directions mirror the probe index so the cascade accumulates
        // irradiance from the correct (rightward/upward) side of the light.
        int seedProbe = (uMirrorProbe > 0.5)
                      ? (int(uProbeCount) - 1 - probeIdx)
                      : probeIdx;
        ivec2 sc = (uSwapCoords > 0.5)
                 ? ivec2(sliceIdx, seedProbe)
                 : ivec2(seedProbe, sliceIdx);
        vec4 scene = texelFetch(uPrevLevel, sc, 0);
        // Beer-Lambert for 1-pixel interval: trans = 1 - alpha.
        return vec4(scene.rgb * scene.a, 1.0 - scene.a);
    } else {
        // Interleaved ray layout: texX = probeIdx * numRays + rayIdx.
        int texX = probeIdx * prevNumRays + rayIdx;
        return texelFetch(uPrevLevel, ivec2(texX, sliceIdx), 0);
    }
}

void main() {
    int texelX   = int(gl_FragCoord.x);
    int sliceIdx = int(gl_FragCoord.y);

    int level       = int(uLevel);
    int numRays     = (1 << level) + 1;
    int levelProbes = (int(uProbeCount) + (1 << level) - 1) >> level;

    int probeIdx = texelX / numRays;
    int rayIdx   = texelX - probeIdx * numRays;

    // Fragments beyond the active region are unused — write transparent black
    // so they never contribute if accidentally sampled.
    if (probeIdx >= levelProbes) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Cross-slice compositing (the "holographic" step).
    // Each ray at level k is built from two rays at level k-1 that cross the
    // slice boundary, giving the probe angular spread in the slice dimension.
    // lower/upper are the two adjacent level-(k-1) ray indices that bracket
    // the current level-k ray.
    int prevInterval = 1 << (level - 1);
    int lower = rayIdx / 2;
    int upper = (rayIdx + 1) / 2;

    // crossA: near probe looks along 'lower', far probe looks along 'upper'
    //         at the slice offset corresponding to 'lower'.
    int sliceOffA = dirToSliceOffset(lower, prevInterval);
    vec4 crossA = compositeRay(
        loadPrev(probeIdx * 2,     lower, sliceIdx),
        loadPrev(probeIdx * 2 + 1, upper, sliceIdx + sliceOffA)
    );

    // crossB: near probe looks along 'upper', far probe looks along 'lower'
    //         at the slice offset corresponding to 'upper'.
    int sliceOffB = dirToSliceOffset(upper, prevInterval);
    vec4 crossB = compositeRay(
        loadPrev(probeIdx * 2,     upper, sliceIdx),
        loadPrev(probeIdx * 2 + 1, lower, sliceIdx + sliceOffB)
    );

    // Average the two cross-compositions to reduce directional bias.
    FragColor = vec4(
        (crossA.rgb + crossB.rgb) * 0.5,
        (crossA.a   + crossB.a  ) * 0.5
    );
}
