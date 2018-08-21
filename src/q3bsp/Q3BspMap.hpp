#ifndef Q3BSPMAP_INCLUDED
#define Q3BSPMAP_INCLUDED

#include "Frustum.hpp"
#include "common/BspMap.hpp"
#include "q3bsp/Q3Bsp.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/Ubo.hpp"
#include <map>
#include <set>
#include <vector>

class  GameTexture;
class  Q3BspBiquadPatch;
struct Q3BspPatch;

/*
 *  Quake III map
 */

class Q3BspMap : public BspMap
{
public:
    static const int   s_tesselationLevel; // level of curved surface tesselation
    static const float s_worldScale;       // scale down factor for the map

    Q3BspMap(bool bspValid) : BspMap(bspValid) {}
    ~Q3BspMap();

    void Init();
    void OnRender();
    void OnUpdate(const Math::Vector3f &cameraPosition);
    void RebuildPipeline();
    std::string ThreadAndBspStats();

    bool ClusterVisible(int cameraCluster, int testCluster)   const;
    int  FindCameraLeaf(const Math::Vector3f &cameraPosition) const;
    void CalculateVisibleFaces(int threadIndex, int startOffset, int cameraLeaf);
    void ToggleRenderFlag(int flag);

    // bsp data
    Q3BspHeader     header;
    Q3BspEntityLump entities;
    std::vector<Q3BspTextureLump>   textures;
    std::vector<Q3BspPlaneLump>     planes;
    std::vector<Q3BspNodeLump>      nodes;
    std::vector<Q3BspLeafLump>      leaves;
    std::vector<Q3BspLeafFaceLump>  leafFaces;
    std::vector<Q3BspLeafBrushLump> leafBrushes;
    std::vector<Q3BspModelLump>     models;
    std::vector<Q3BspBrushLump>     brushes;
    std::vector<Q3BspBrushSideLump> brushSides;
    std::vector<Q3BspVertexLump>    vertices;
    std::vector<Q3BspMeshVertLump>  meshVertices;
    std::vector<Q3BspEffectLump>    effects;
    std::vector<Q3BspFaceLump>      faces;
    std::vector<Q3BspLightMapLump>  lightMaps;
    std::vector<Q3BspLightVolLump>  lightVols;
    Q3BspVisDataLump                visData;
private:
    void LoadTextures();
    void LoadLightmaps();
    void SetLightmapGamma(float gamma);
    void CreatePatch(const Q3BspFaceLump &f);

    // queue data for drawing
    void Draw(int threadIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

    // Vulkan buffer creation
    void CreateDescriptorsForFace(const Q3BspFaceLump &face, int idx, int vertexOffset, int indexOffset);
    void CreateDescriptorsForPatch(int idx, int &vertexOffset, int &indexOffset, std::vector<Q3BspBiquadPatch*> &vdata, std::vector<int> &idata);
    void CreateFaceBuffers(const std::vector<Q3BspFaceLump*> &faceData, int vertexCount, int indexCount);
    void CreatePatchBuffers(const std::vector<Q3BspBiquadPatch*> &patchData, int vertexCount, int indexCount);
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool(uint32_t numDescriptors);
    void CreateDescriptor(const vk::Texture **textures, vk::Descriptor *descriptor);

    // render data
    std::vector<Q3LeafRenderable>   m_renderLeaves;   // bsp leaves in "renderable format"
    std::vector<Q3FaceRenderable>   m_renderFaces;    // bsp faces in "renderable format"
    std::vector<Q3BspPatch *>       m_patches;        // curved surfaces
    std::vector<GameTexture *>      m_textures;       // loaded in-game textures
    std::set<Q3FaceRenderable *> m_visibleFaces;      // list of visible surfaces to render
    std::set<int> m_visiblePatches;                   // list of visible patches to render
    std::vector<std::set<Q3FaceRenderable *>> m_visibleFacesPerThread;   // list of visible surfaces to render (per thread)
    std::vector<std::set<int>>                m_visiblePatchesPerThread; // list of visible patches to render (per thread)
    vk::Texture *m_lightmapTextures = nullptr;        // bsp lightmaps

    Frustum  m_frustum; // view frustum

    // helper textures
    GameTexture *m_missingTex = nullptr; // rendered if an in-game texture is missing
    vk::Texture  m_whiteTex;             // used if no lightmap specified for a face

    // rendering Vulkan buffers and pipelines
    RenderBuffers m_renderBuffers;
    UniformBufferObject m_ubo;
    vk::Pipeline   m_facesPipeline; // used for rendering standard faces
    vk::Pipeline   m_patchPipeline; // used for rendering curves/patches

    // all faces and patches use shared vertex buffer info and descriptor set layout
    vk::VertexBufferInfo  m_vbInfo;
    VkDescriptorSetLayout m_dsLayout;
    VkDescriptorPool      m_descriptorPool;

    // store faces and patches in separate buffers
    vk::Buffer m_faceVertexBuffer;
    vk::Buffer m_faceIndexBuffer;
    vk::Buffer m_patchVertexBuffer;
    vk::Buffer m_patchIndexBuffer;

    // secondary command buffers and respective command pools used for rendering - one per thread
    std::vector<VkCommandPool> m_commandPools;
    std::vector<VkCommandBuffer> m_commandBuffers;
    unsigned int m_facesPerThread;
};

#endif
