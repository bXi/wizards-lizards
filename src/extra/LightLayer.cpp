#include "LightLayer.h"
#include "luminoveau.h"
#include "SDL3/SDL_gpu.h"
#include <algorithm>
#include <cmath>
#include <vector>

// ── Internal helpers ──

static int ceilDivI(int a, int b) { return (a + b - 1) / b; }
static int nextPow2(int n)        { int v = 1; while (v < n) v <<= 1; return v; }
static int ceilLog2(int n)        { int l = 0, v = 1; while (v < n) { v <<= 1; l++; } return l; }

static uint32_t rayRowW(int level, uint32_t pc) {
    if (level == 0) return pc;
    int lp = (int(pc) + (1 << level) - 1) >> level;
    return uint32_t(lp) * uint32_t((1 << level) + 1);
}

// ── Uniform structs ──

struct SeedUniforms {
    float    transformX[4];
    float    transformY[4];
    uint32_t probeCount;
    uint32_t sliceCount;
    float    screenW;
    float    screenH;
    float    probeSpacing;
    float    _pad[3];
};

struct ExtendUniforms {
    uint32_t probeCount;
    uint32_t level;
    float    invNumRays;
    uint32_t prevRayW;
    uint32_t currRayW;
    uint32_t sliceCount;
    uint32_t _pad2;
    uint32_t _pad3;
};

struct MergeUniforms {
    uint32_t probeCount;
    uint32_t numDirections;
    uint32_t levelProbes;
    uint32_t numRays;
    uint32_t isTopCascade;
    uint32_t fluenceW;
    uint32_t fluenceStride;
    uint32_t level;
    uint32_t mergeStride;
    uint32_t mergeInWidth;
    uint32_t direction;
    uint32_t fluenceH;
    uint32_t sliceCount;
    float    ambientR;
    float    ambientG;
    float    ambientB;
    uint32_t clearFluence;
};

struct BlurUniforms {
    uint32_t w, h, stride, screenW, screenH;
    uint32_t _pad0, _pad1, _pad2;
};

struct ClearParams {
    uint32_t count;
    uint32_t _pad0, _pad1, _pad2;
};

struct BounceParams {
    uint32_t screenW;
    uint32_t screenH;
    float    albedo;
    float    _pad;
};

// ── Framebuffer accessors ──

SDL_GPUTexture* LightLayer::_sceneTex()    const { return Renderer::GetFramebuffer("ll_scene_framebuffer")->fbContent; }
SDL_GPUTexture* LightLayer::_fluenceTex()  const { return Renderer::GetFramebuffer("ll_fluence_framebuffer")->fbContent; }
Texture         LightLayer::_fluenceTexV() const { return Renderer::GetFramebuffer("ll_fluence_framebuffer")->textureView; }
SDL_GPUTexture* LightLayer::_bounceTex()   const { return Renderer::GetFramebuffer("ll_bounce_framebuffer")->fbContent; }
Texture         LightLayer::_outputTexV()  const { return Renderer::GetFramebuffer("ll_output_framebuffer")->textureView; }

// ── _init ──

void LightLayer::_init(int probeTarget) {
    uint32_t physW = uint32_t(Window::GetPhysicalWidth());
    uint32_t physH = uint32_t(Window::GetPhysicalHeight());
    m_rtW  = float(Window::GetWidth());
    m_rtH  = float(Window::GetHeight());
    m_texW = float(physW);
    m_texH = float(physH);

    int maxDim      = int(std::max(physW, physH));
    m_probesX       = uint32_t(std::max(2, ceilDivI(int(physW) * probeTarget, maxDim)));
    m_probesY       = uint32_t(std::max(2, ceilDivI(int(physH) * probeTarget, maxDim)));
    m_probesMax     = std::max(m_probesX, m_probesY);
    m_mergeStride   = uint32_t(nextPow2(int(m_probesMax)));
    m_numCascades   = uint32_t(ceilLog2(int(m_probesMax)));
    m_fluenceStride = m_probesX;
    m_probeSpacing  = float(maxDim) / float(probeTarget);
    m_sceneClear    = true;

    m_clearPipeline  = AssetHandler::GetComputePipeline("assets/shaders/hrc_clear.comp");
    m_seedPipeline   = AssetHandler::GetComputePipeline("assets/shaders/hrc_seed.comp");
    m_extendPipeline = AssetHandler::GetComputePipeline("assets/shaders/hrc_extend.comp");
    m_mergePipeline  = AssetHandler::GetComputePipeline("assets/shaders/hrc_merge.comp");
    m_blurPipeline   = AssetHandler::GetComputePipeline("assets/shaders/hrc_blur.comp");
    m_bouncePipeline = AssetHandler::GetComputePipeline("assets/shaders/hrc_bounce.comp");

    const SDL_GPUBufferUsageFlags STORAGE_RW =
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;

    for (uint32_t i = 0; i < m_numCascades; i++) {
        uint32_t sz  = rayRowW(int(i), m_probesMax) * m_probesMax * 8u;
        m_rayBufs[i] = Compute::CreateBuffer(sz, STORAGE_RW);
    }

    uint32_t mergeSize   = m_mergeStride * m_probesMax * 4u;
    uint32_t fluenceSize = m_probesX * m_probesY * 4u;
    m_mergeBufs[0] = Compute::CreateBuffer(mergeSize,   STORAGE_RW);
    m_mergeBufs[1] = Compute::CreateBuffer(mergeSize,   STORAGE_RW);
    m_fluenceBuf   = Compute::CreateBuffer(fluenceSize, STORAGE_RW);

    {
        std::vector<uint8_t> zeros(std::max(mergeSize, fluenceSize), 0u);
        Compute::UploadBufferData(m_mergeBufs[0], zeros.data(), mergeSize);
        Compute::UploadBufferData(m_mergeBufs[1], zeros.data(), mergeSize);
        Compute::UploadBufferData(m_fluenceBuf,   zeros.data(), fluenceSize);
    }

    SpriteRenderTargetConfig sceneCfg;
    sceneCfg.clearOnLoad = true;
    sceneCfg.clearColor  = 0x00000000;
    sceneCfg.blendMode   = BlendMode::None;
    sceneCfg.width       = physW;
    sceneCfg.height      = physH;
    Renderer::CreateSpriteRenderTarget("ll_scene", sceneCfg);

    SpriteRenderTargetConfig fluenceCfg;
    fluenceCfg.clearOnLoad = false;
    fluenceCfg.blendMode   = BlendMode::None;
    fluenceCfg.width       = m_probesX;
    fluenceCfg.height      = m_probesY;
#ifndef LUMINOVEAU_SHADER_BACKEND_OPENGL
    fluenceCfg.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
#endif
    Renderer::CreateSpriteRenderTarget("ll_fluence", fluenceCfg);

    SpriteRenderTargetConfig bounceCfg;
    bounceCfg.clearOnLoad = false;
    bounceCfg.blendMode   = BlendMode::None;
    bounceCfg.width       = m_probesX;
    bounceCfg.height      = m_probesY;
#ifndef LUMINOVEAU_SHADER_BACKEND_OPENGL
    bounceCfg.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
#endif
    Renderer::CreateSpriteRenderTarget("ll_bounce", bounceCfg);

    // ll_output must match primaryFramebuffer dimensions exactly.
    // renderFrameBuffer reuses the same rtt_uniforms (uMax=physW/primaryFB.width)
    // for all renderToScreen passes — if ll_output is smaller than primaryFB,
    // the UV sub-range is wrong and the content appears offset/scaled.
    auto* primaryFB = Renderer::GetFramebuffer("primaryFramebuffer");

    SpriteRenderTargetConfig outputCfg;
    outputCfg.clearOnLoad    = true;
    outputCfg.clearColor     = 0x00000000;
    outputCfg.blendMode      = BlendMode::Additive;
    outputCfg.renderToScreen = true;
    outputCfg.width          = primaryFB->width;
    outputCfg.height         = primaryFB->height;
    Renderer::CreateSpriteRenderTarget("ll_output", outputCfg);

    Shader vert     = AssetHandler::GetShader("assets/shaders/effect_passthrough.vert");
    Shader blitFrag = AssetHandler::GetShader("assets/shaders/hrc_blit.frag");
    m_blitEffect    = Effects::Create(vert, blitFrag);
}

// ── _close ──

void LightLayer::_close() {
    for (int i = 0; i < MAX_CASCADE_LEVELS; i++) {
        Compute::DestroyBuffer(m_rayBufs[i]);
        m_rayBufs[i] = nullptr;
    }
    Compute::DestroyBuffer(m_mergeBufs[0]); m_mergeBufs[0] = nullptr;
    Compute::DestroyBuffer(m_mergeBufs[1]); m_mergeBufs[1] = nullptr;
    Compute::DestroyBuffer(m_fluenceBuf);   m_fluenceBuf   = nullptr;

    Renderer::RemoveSpriteRenderTarget("ll_scene");
    Renderer::RemoveSpriteRenderTarget("ll_fluence");
    Renderer::RemoveSpriteRenderTarget("ll_bounce");
    Renderer::RemoveSpriteRenderTarget("ll_output");
}

// ── _startLight ──

void LightLayer::_startLight() {
    Draw::SetTargetRenderPass("ll_scene");
    m_sceneClear = false;
}

// ── _endLight ──

void LightLayer::_endLight() {
    Draw::ResetTargetRenderPass();
}

// ── _draw ──
// Queues cascade compute + blit render pass, then draws the output.
// Call once per frame after all StartLight/EndLight pairs.

void LightLayer::_draw(vf2d pos, vf2d size) {
    if (m_sceneClear) return; // no StartLight this frame — nothing to render

    // Bounce: convert previous frame's fluence → re-emission texture
    {
        BounceParams bp{};
        bp.screenW = uint32_t(m_texW);
        bp.screenH = uint32_t(m_texH);
        bp.albedo  = m_bounces ? m_bounceAlbedo : 0.0f;

        Compute::SetPipeline(m_bouncePipeline);
        Compute::BindReadTexture(0, _fluenceTex());
        Compute::BindReadTexture(1, _sceneTex());
        Compute::BindReadWriteTexture(0, _bounceTex());
        Compute::PushUniform(0, &bp, sizeof(bp));
        Compute::DispatchAuto(m_probesX, m_probesY, 1);
    }

    // Clear fluence accumulation buffer
    {
        ClearParams cp{ m_probesX * m_probesY, 0u, 0u, 0u };
        Compute::SetPipeline(m_clearPipeline);
        Compute::BindReadWriteBuffer(0, m_fluenceBuf);
        Compute::PushUniform(0, &cp, sizeof(cp));
        Compute::DispatchAuto(m_probesX * m_probesY, 1, 1);
    }

    // Cascade (4 cardinal directions)
    _queueCascade(0, m_probesX, m_probesY); // E
    _queueCascade(1, m_probesY, m_probesX); // N
    _queueCascade(2, m_probesX, m_probesY); // W
    _queueCascade(3, m_probesY, m_probesX); // S

    // Blur + convert (SSBO RGB9E5 → rgba16f texture)
    {
        BlurUniforms bu{};
        bu.w       = m_probesX;
        bu.h       = m_probesY;
        bu.stride  = m_fluenceStride;
        bu.screenW = uint32_t(m_texW);
        bu.screenH = uint32_t(m_texH);

        Compute::SetPipeline(m_blurPipeline);
        Compute::BindReadTexture(0, _sceneTex());
        Compute::BindReadBuffer(0, m_fluenceBuf);
        Compute::BindReadWriteTexture(0, _fluenceTex());
        Compute::PushUniform(0, &bu, sizeof(bu));
        Compute::DispatchAuto(m_probesX, m_probesY, 1);
    }

    // Blit: composite scene + fluence → output RT
    m_blitEffect["uExposure"] = m_exposure;
    m_blitEffect["uTexW"]     = m_texW;
    m_blitEffect["uTexH"]     = m_texH;

    Draw::SetTargetRenderPass("ll_output");
    Draw::SetEffect(m_blitEffect);
    Draw::SetEffectTexture(0, Renderer::GetFramebuffer("ll_scene_framebuffer")->textureView,
                           ScaleMode::NEAREST);
    Draw::SetEffectTexture(1, _fluenceTexV(), ScaleMode::LINEAR);
    Draw::Texture(Renderer::WhitePixel(), {0.0f, 0.0f}, {m_rtW, m_rtH});
    Draw::ClearEffects();
    Draw::ClearEffectTextures();
    Draw::ResetTargetRenderPass();

    // renderToScreen=true on ll_output: renderFrameBuffer blits it to swapchain
    // in NDC space — no world/camera dependency, always covers full screen.

    // Mark scene for clearing on next frame's first StartLight
    m_sceneClear = true;
}

// ── _queueCascade ──

void LightLayer::_queueCascade(int dir, uint32_t pc, uint32_t sc) {
    const float s     = m_probeSpacing;
    const float physW = m_texW;
    const float physH = m_texH;

    float txX[4] = {}, txY[4] = {};
    switch (dir) {
        case 0: txX[0]=s;  txX[2]=0;     txY[1]=s; txY[2]=0;     break; // E
        case 1: txX[1]=s;  txX[2]=0;     txY[0]=s; txY[2]=0;     break; // N
        case 2: txX[0]=-s; txX[2]=physW; txY[1]=s; txY[2]=0;     break; // W
        case 3: txX[1]=s;  txX[2]=0;     txY[0]=-s;txY[2]=physH; break; // S
    }

    SeedUniforms su{};
    memcpy(su.transformX, txX, sizeof(txX));
    memcpy(su.transformY, txY, sizeof(txY));
    su.probeCount   = pc;
    su.sliceCount   = sc;
    su.screenW      = physW;
    su.screenH      = physH;
    su.probeSpacing = s;

    Compute::SetPipeline(m_seedPipeline);
    Compute::BindReadTexture(0, _sceneTex());
    Compute::BindReadTexture(1, _bounceTex());
    Compute::BindReadWriteBuffer(0, m_rayBufs[0]);
    Compute::PushUniform(0, &su, sizeof(su));
    Compute::DispatchAuto(pc, sc, 1);

    for (uint32_t k = 1; k < m_numCascades; k++) {
        int      interval    = 1 << int(k);
        int      numRays     = interval + 1;
        uint32_t levelProbes = uint32_t((int(pc) + interval - 1) >> int(k));
        uint32_t currRayW    = rayRowW(int(k),   pc);
        uint32_t prevRayW    = rayRowW(int(k)-1, pc);

        ExtendUniforms eu{};
        eu.probeCount = pc;
        eu.level      = k;
        eu.invNumRays = 1.0f / float(numRays);
        eu.prevRayW   = prevRayW;
        eu.currRayW   = currRayW;
        eu.sliceCount = sc;

        Compute::SetPipeline(m_extendPipeline);
        Compute::BindReadBuffer(0, m_rayBufs[k - 1]);
        Compute::BindReadWriteBuffer(0, m_rayBufs[k]);
        Compute::PushUniform(0, &eu, sizeof(eu));
        Compute::DispatchAuto(levelProbes * uint32_t(numRays), sc, 1);
    }

    int      readIdx        = 1;
    int      writeIdx       = 0;
    uint32_t prevMergeWidth = 0;

    for (int step = 0; step < int(m_numCascades); step++) {
        int      level         = int(m_numCascades) - 1 - step;
        bool     isTopCascade  = (step == 0);
        bool     isFluence     = (level == 0);
        int      numDirections = 1 << level;
        int      numRays       = (level == 0) ? 1 : ((1 << level) + 1);
        uint32_t levelProbes   = uint32_t((int(pc) + (1 << level) - 1) >> level);
        uint32_t dispatchX     = levelProbes * uint32_t(numDirections);

        MergeUniforms mu{};
        mu.probeCount    = pc;
        mu.numDirections = uint32_t(numDirections);
        mu.levelProbes   = levelProbes;
        mu.numRays       = uint32_t(numRays);
        mu.isTopCascade  = isTopCascade ? 1u : 0u;
        mu.fluenceW      = m_probesX;
        mu.fluenceStride = m_fluenceStride;
        mu.level         = uint32_t(level);
        mu.mergeStride   = m_mergeStride;
        mu.mergeInWidth  = isTopCascade ? 0u : prevMergeWidth;
        mu.direction     = uint32_t(dir);
        mu.fluenceH      = m_probesY;
        mu.sliceCount    = sc;
        mu.ambientR      = m_ambient ? m_ambientColor[0] : 0.0f;
        mu.ambientG      = m_ambient ? m_ambientColor[1] : 0.0f;
        mu.ambientB      = m_ambient ? m_ambientColor[2] : 0.0f;
        mu.clearFluence  = 0u;

        Compute::SetPipeline(m_mergePipeline);
        Compute::BindReadBuffer(0, m_rayBufs[level]);
        Compute::BindReadBuffer(1, m_mergeBufs[readIdx]);
        Compute::BindReadWriteBuffer(0, m_mergeBufs[writeIdx]);
        Compute::BindReadWriteBuffer(1, m_fluenceBuf);
        Compute::PushUniform(0, &mu, sizeof(mu));
        Compute::DispatchAuto(dispatchX, sc, 1);

        prevMergeWidth = dispatchX;
        if (!isFluence) std::swap(readIdx, writeIdx);
    }
}
