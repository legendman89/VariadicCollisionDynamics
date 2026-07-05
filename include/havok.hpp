#pragma once

#include "plugin.hpp"

#include <cstdint>

namespace VCD::Havok {

    struct StridedVertices
    {
        const float* vertices{ nullptr };
        int numVertices{ 0 };
        int striding{ 0 };

        StridedVertices(const RE::hkVector4* a_vertices, const int& a_numVertices) :
            vertices(reinterpret_cast<const float*>(a_vertices)),
            numVertices(a_numVertices),
            striding(sizeof(RE::hkVector4))
        {}
    };

    // Based on flyingparticle's RE.
    struct ConvexVerticesBuildConfig
    {
        bool createConnectivity;
        bool shrinkByConvexRadius;
        bool useOptimizedShrinking;
        float convexRadius;
        std::int32_t maxVertices;
        float maxRelativeShrink;
        float maxShrinkingVerticesDisplacement;
        float maxCosAngleForBevelPlanes;
    };
    static_assert(sizeof(ConvexVerticesBuildConfig) == 0x18);

    using GetOriginalVertices_t = void (*)(const RE::hkpConvexVerticesShape*, RE::hkArray<RE::hkVector4>&);
    inline REL::Relocation<GetOriginalVertices_t> GetOriginalVertices{ RELOCATION_ID(64067, 65093) };

    using ConvexVerticesShapeCtor_t = void (*)(RE::hkpConvexVerticesShape*, const StridedVertices&, const ConvexVerticesBuildConfig&);
    inline REL::Relocation<ConvexVerticesShapeCtor_t> ConvexVerticesShapeCtor{ RELOCATION_ID(78843, 80831) };

    inline RE::hkpConvexVerticesShape* AllocateConvexVerticesShape()
    {
        auto* router = RE::hkMemoryRouter::GetInstancePtr();
        if (!router || !router->Heap) {
            return nullptr;
        }

        return reinterpret_cast<RE::hkpConvexVerticesShape*>(router->Heap->BlockAlloc(sizeof(RE::hkpConvexVerticesShape)));
    }
}
