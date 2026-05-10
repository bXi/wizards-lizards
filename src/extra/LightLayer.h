#pragma once

#include "luminoveau.h"
#include "utils/vectors.h"
#include <cstdint>

// 2D global-illumination layer using Hybrid Radiance Cascades.
// Singleton — access via static methods.
//
// Per-frame usage:
//   LightLayer::StartLight();
//       Draw::CircleFilled(...);       // emissive light source
//       Draw::RectangleFilled(...);    // opaque occluder
//   LightLayer::EndLight();
//
//   LightLayer::StartLight();          // multiple pairs accumulate
//       Draw::CircleFilled(...);
//   LightLayer::EndLight();
//
//   LightLayer::Draw({0,0}, {w, h});   // composite + render output

class LightLayer {
public:
    // ── Lifecycle ──
    static void Init(int probeTarget = 1024) { get()._init(probeTarget); }
    static void Close()                      { get()._close(); }

    // ── Per-frame API ──
    static void StartLight()                 { get()._startLight(); }
    static void EndLight()                   { get()._endLight(); }
    static void Draw(vf2d pos, vf2d size)    { get()._draw(pos, size); }

    // ── Settings (change freely each frame) ──
    static void SetExposure(float v)                              { get().m_exposure = v; }
    static void SetBounces(bool v)                                { get().m_bounces = v; }
    static void SetBounceAlbedo(float v)                          { get().m_bounceAlbedo = v; }
    static void SetAmbient(bool v)                                { get().m_ambient = v; }
    static void SetAmbientColor(float r, float g, float b)        { get().m_ambientColor[0]=r; get().m_ambientColor[1]=g; get().m_ambientColor[2]=b; }
    static void SetFalloffGamma(float v)                          { get().m_falloffGamma = v; }
    static void SetFalloffShift(float v)                          { get().m_falloffShift = v; }

    static float GetExposure()      { return get().m_exposure; }
    static bool  GetBounces()       { return get().m_bounces; }
    static bool  GetAmbient()       { return get().m_ambient; }
    static float GetFalloffGamma()  { return get().m_falloffGamma; }
    static float GetFalloffShift()  { return get().m_falloffShift; }

    // Pass name for sprites that should render above the HRC light overlay.
    static constexpr const char* ForegroundPass = "2dsprites_fg";
    static constexpr const char* UIPass         = "2dsprites_ui";

    // ── Singleton boilerplate ──
    LightLayer(const LightLayer&) = delete;
    static LightLayer& get() { static LightLayer instance; return instance; }

private:
    LightLayer() = default;

    void _init(int probeTarget);
    void _close();
    void _startLight();
    void _endLight();
    void _draw(vf2d pos, vf2d size);
    void _queueCascade(int dir, uint32_t pc, uint32_t sc);

    SDL_GPUTexture* _sceneTex()    const;
    SDL_GPUTexture* _fluenceTex()  const;
    Texture         _fluenceTexV() const;
    SDL_GPUTexture* _bounceTex()   const;
    Texture         _outputTexV()  const;

    // ── Compute pipelines ──
    ComputePipelineAsset m_clearPipeline;
    ComputePipelineAsset m_seedPipeline;
    ComputePipelineAsset m_extendPipeline;
    ComputePipelineAsset m_mergePipeline;
    ComputePipelineAsset m_blurPipeline;
    ComputePipelineAsset m_bouncePipeline;

    // ── Blit effect ──
    EffectAsset m_blitEffect;

    // ── GPU storage buffers ──
    static constexpr int MAX_CASCADE_LEVELS = 12;
    SDL_GPUBuffer* m_rayBufs[MAX_CASCADE_LEVELS] = {};
    SDL_GPUBuffer* m_mergeBufs[2]                = {};
    SDL_GPUBuffer* m_fluenceBuf                  = nullptr;

    // ── Cascade dimensions ──
    uint32_t m_probesMax     = 0;
    uint32_t m_mergeStride   = 0;
    uint32_t m_numCascades   = 0;
    uint32_t m_probesX       = 0;
    uint32_t m_probesY       = 0;
    uint32_t m_fluenceStride = 0;
    float    m_probeSpacing  = 1.0f;

    // ── Frame dimensions ──
    float m_rtW = 512.0f;
    float m_rtH = 512.0f;
    float m_texW = 512.0f;
    float m_texH = 512.0f;

    // ── Frame state ──
    bool m_sceneClear = true; // true = scene RT needs clearing on next StartLight

    // ── Settings ──
    float m_exposure        = 2.0f;
    bool  m_bounces         = true;
    float m_bounceAlbedo    = 0.5f;
    bool  m_ambient         = false;
    float m_ambientColor[3] = { 0.02f, 0.02f, 0.04f };
    float m_falloffGamma    = 1.0f;
    float m_falloffShift    = 0.0f;
};
