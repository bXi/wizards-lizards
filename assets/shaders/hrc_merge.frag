#version 450

// Holographic Radiance Cascade — Merge pass
// Resolves one cascade level top-down.  Each pass reads:
//   uRay     — the extend output for this level (or the scene texture at level 0)
//   uMergeIn — the resolved irradiance from the next-coarser level (unused at top)
// and writes merged irradiance at this level's probe/angular-bin resolution.
//
// Fragment coordinate layout (same across all levels):
//   x = outX = probeIdx * numDirections + angBinIdx
//   y = sliceIdx
//
// At level 0: numDirections=1, so outX = probeIdx = screen x (for H dirs).
//             This RT is the per-direction fluence written to hrc_fluence_*.
//
// COMPUTE NOTE: With compute shaders this reads/writes storage buffers
// (array<u32> RGB9E5 for merged fluence, array<vec2u> F16 for ray data).
// The index arithmetic is identical; replace texelFetch with buf[...] loads.
//
// Even-probe Richardson extrapolation is implemented: even-indexed probes
// (at levels > 0) composite with their odd neighbour before merging with the
// coarser level, then average the result with the standard path to halve the
// block artifact that arises from the coarse cascade intervals.

layout(set = 2, binding = 0) uniform sampler2D uRay;
// Extend RT for this level (RGBA16F: rad.rgb, trans).
// At level 0: the scene render target (RGB=emissive, A=opacity) is bound here.

layout(set = 2, binding = 1) uniform sampler2D uMergeIn;
// Merge output from the next-coarser cascade level (same RGBA16F layout).
// Ignored when uIsTopCascade == 1.0; ambient/sky colour is used instead.

layout(set = 3, binding = 0) uniform MergeUniforms {
    float uLevel;         // cascade level (0 = finest / most probes)
    float uProbeCount;    // probes for this direction
    float uSliceCount;    // slices for this direction
    float uIsTopCascade;  // 1.0 → use ambient fallback, no mergeIn read
    float uSwapCoords;    // 1.0 for vertical directions (N/S)
    float uAmbientR;
    float uAmbientG;
    float uAmbientB;
    float uMirrorProbe;   // 1.0 for W/S: mirror probe index when seeding from scene
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(location = 0) in  vec2 tex_coords;
layout(location = 0) out vec4 FragColor;

#define PI 3.14159265359

int dirToSliceOffset(int dirIdx, int intervalSize) {
    return dirIdx * 2 - intervalSize;
}

// Exact angular arc (steradians in 2D) for subBin f at this cascade level.
// Derived from the HRC paper's Eq. 13.
// Total arc per direction = PI/2 (four directions cover the full circle = 2*PI).
float coneArc(int subBin, int numDirections) {
    // d = 2 * numDirections (total sub-bins at this level)
    float d = float(numDirections * 2);
    float lo = float(subBin * 2    ) - d;   // = 2*subBin - 2*numDir
    float hi = float(subBin * 2 + 2) - d;   // = 2*subBin - 2*numDir + 2
    return atan(hi / d) - atan(lo / d);
}

// Load (rad.rgb, transmittance) from the extend RT (or scene seed at level 0).
vec4 loadRay(int probeIdx, int rayIdx, int sliceIdx) {
    int level       = int(uLevel);
    int numRays     = (level == 0) ? 1 : ((1 << level) + 1);
    int levelProbes = (int(uProbeCount) + (1 << level) - 1) >> level;

    if (probeIdx < 0 || probeIdx >= levelProbes ||
        rayIdx   < 0 || rayIdx   >= numRays      ||
        sliceIdx < 0 || sliceIdx >= int(uSliceCount)) {
        return vec4(0.0, 0.0, 0.0, 1.0); // no radiance, fully transparent
    }

    if (level == 0) {
        // Read scene texture as seed.
        // W/S directions mirror the probe index so level-0 reads the correct side.
        int seedProbe = (uMirrorProbe > 0.5)
                      ? (int(uProbeCount) - 1 - probeIdx)
                      : probeIdx;
        ivec2 sc = (uSwapCoords > 0.5)
                 ? ivec2(sliceIdx, seedProbe)
                 : ivec2(seedProbe, sliceIdx);
        vec4 scene = texelFetch(uRay, sc, 0);
        return vec4(scene.rgb * scene.a, 1.0 - scene.a);
    } else {
        int texX = probeIdx * numRays + rayIdx;
        return texelFetch(uRay, ivec2(texX, sliceIdx), 0);
    }
}

// Composite two ray segments (Porter-Duff "over" for transmittance/radiance).
// Paper Eq. 7:  ⟨r_n + t_n · r_f,  t_n · t_f⟩
// Stored as vec4(rad.rgb, transmittance).
vec4 compositeRay(vec4 near, vec4 far) {
    return vec4(near.rgb + far.rgb * near.a, near.a * far.a);
}

// Load irradiance from the coarser-level merge result.
// Falls back to ambient sky colour when out of bounds or at the top cascade.
vec3 loadMergeIn(int texX, int sliceIdx) {
    vec3 sky = vec3(uAmbientR, uAmbientG, uAmbientB);
    if (uIsTopCascade > 0.5)
        return sky;
    if (texX < 0 || texX >= int(uProbeCount) ||
        sliceIdx < 0 || sliceIdx >= int(uSliceCount))
        return sky;
    return texelFetch(uMergeIn, ivec2(texX, sliceIdx), 0).rgb;
}

void main() {
    int texelX   = int(gl_FragCoord.x);
    int sliceIdx = int(gl_FragCoord.y);

    int level         = int(uLevel);
    int numDirections = 1 << level;          // angular bins per probe
    int probeIdx      = texelX >> level;     // = texelX / numDirections
    int angBinIdx     = texelX & (numDirections - 1);

    int levelProbes = (int(uProbeCount) + (1 << level) - 1) >> level;
    if (probeIdx >= levelProbes) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 result = vec3(0.0);

    // Even-probe Richardson extrapolation: only applies at levels > 0 (at
    // level 0 each probe is a single scene pixel; compositing with the
    // neighbour there would just bleed adjacent pixels, not fix cascade error).
    bool isEven = (level > 0) && ((probeIdx & 1) == 0);

    // Accumulate two sub-bins per angular bin (one on each side of centre).
    for (int side = 0; side < 2; side++) {
        int subBin = angBinIdx * 2 + side;
        int rayIdx = angBinIdx + side;

        float weight = coneArc(subBin, numDirections);

        vec4 ray = loadRay(probeIdx, rayIdx, sliceIdx);

        // Slice offset encodes which parallel sweep-line carries the ray's
        // far-field continuation in the coarser cascade.
        int sliceOff = dirToSliceOffset(rayIdx, numDirections);

        // farStep: even probes composite with their neighbour and look 2
        // intervals ahead; odd probes do the simple single-interval path.
        int farStep  = isEven ? 2 : 1;
        int farX     = ((probeIdx + farStep) << level) + subBin;
        int farSlice = sliceIdx + sliceOff * farStep;
        vec3 farFluence = loadMergeIn(farX, farSlice);

        if (isEven) {
            // Even-probe Richardson extrapolation (reference Eq. parity path):
            // Composite current probe with its odd neighbour to span 2 intervals.
            vec4 ext  = loadRay(probeIdx + 1, rayIdx, sliceIdx + sliceOff);
            vec3 cRad = ray.rgb + ext.rgb * ray.a;
            float cTrans = ray.a * ext.a;

            // Merged contribution: fine composited ray + far-field 2 steps out.
            vec3 merged = cRad * weight + farFluence * cTrans;

            // Coarser level's own estimate at this probe's position.
            // (probeIdx << level) + subBin maps to the parent coarse probe's
            // angular bin in the coarser merge buffer — the same data the odd
            // neighbours also share, so averaging with it halves the block error.
            int coarseX = (probeIdx << level) + subBin;
            vec3 coarseFluence = loadMergeIn(coarseX, sliceIdx);

            result += (merged + coarseFluence) * 0.5;
        } else {
            // Odd-probe: standard single-interval merge.
            result += ray.rgb * weight + farFluence * ray.a;
        }
    }

    FragColor = vec4(result, 1.0);
}
