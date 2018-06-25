#ifndef Q3BSPRENDERHELPERS_INCLUDED
#define Q3BSPRENDERHELPERS_INCLUDED

#include "renderer/vulkan/Base.hpp"
#include "renderer/vulkan/Buffers.hpp"
#include <vector>
#include <map>

/*
 * Helper structs for Q3BspMap rendering
 */

enum Q3BspRenderFlags
{
    Q3RenderShowWireframe  = 1 << 0,
    Q3RenderShowLightmaps  = 1 << 1,
    Q3RenderUseLightmaps   = 1 << 2,
    Q3RenderAlphaTest      = 1 << 3,
    Q3RenderSkipMissingTex = 1 << 4,
    Q3RenderSkipPVS        = 1 << 5,
    Q3RenderSkipFC         = 1 << 6,
    Q3Multisampling        = 1 << 7
};


// leaf structure used for occlusion culling (PVS/frustum)
struct Q3LeafRenderable
{
    int visCluster = 0;
    int firstFace  = 0;
    int numFaces   = 0;
    Math::Vector3f boundingBoxVertices[8];
};


// face structure used for rendering
struct Q3FaceRenderable
{
    int type  = 0;
    int index = 0;
};


// Vulkan buffers and descriptor for a single face in the BSP
struct FaceBuffers
{
    vk::Descriptor descriptor;
    int vertexCount = 0;
    int indexCount  = 0;
    int vertexOffset = 0;
    int indexOffset  = 0;
};


struct RenderBuffers
{
    vk::Buffer uniformBuffer;

    std::map<int, FaceBuffers> m_faceBuffers;
    std::map<int, std::vector<FaceBuffers> > m_patchBuffers;
};


// map statistics
struct BspStats
{
    int totalVertices   = 0;
    int totalFaces      = 0;
    int visibleFaces    = 0;
    int totalPatches    = 0;
    int visiblePatches  = 0;
};

#endif