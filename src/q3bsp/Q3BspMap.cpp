#include "q3bsp/Q3BspMap.hpp"
#include "q3bsp/Q3BspPatch.hpp"
#include "renderer/TextureManager.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "renderer/vulkan/Pipeline.hpp"
#include "Math.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <sstream>

extern RenderContext  g_renderContext;
const int   Q3BspMap::s_tesselationLevel = 10;   // level of curved surface tesselation
const float Q3BspMap::s_worldScale       = 64.f; // scale down factor for the map

Q3BspMap::~Q3BspMap()
{
    delete[] entities.ents;
    delete[] visData.vecs;

    for (auto &it : m_patches)
        delete it;

    // release all allocated Vulkan resources
    vkDeviceWaitIdle(g_renderContext.device.logical);
    vk::destroyPipeline(g_renderContext.device, m_facesPipeline);
    vk::destroyPipeline(g_renderContext.device, m_patchPipeline);

    for (auto &it : m_renderBuffers.m_faceBuffers)
    {
        vkDestroyDescriptorPool(g_renderContext.device.logical, it.second.descriptor.pool, nullptr);
        vk::freeBuffer(g_renderContext.device, it.second.vertexBuffer);
        vk::freeBuffer(g_renderContext.device, it.second.indexBuffer);
    }

    for (auto &it : m_renderBuffers.m_patchBuffers)
    {
        for (auto &it2 : it.second)
        {
            vkDestroyDescriptorPool(g_renderContext.device.logical, it2.descriptor.pool, nullptr);
            vk::freeBuffer(g_renderContext.device, it2.vertexBuffer);
            vk::freeBuffer(g_renderContext.device, it2.indexBuffer);
        }
    }

    for (size_t i = 0; i < lightMaps.size(); ++i)
    {
        vk::releaseTexture(g_renderContext.device, m_lightmapTextures[i]);
    }
    delete[] m_lightmapTextures;

    vk::freeBuffer(g_renderContext.device, m_renderBuffers.uniformBuffer);
    vk::releaseTexture(g_renderContext.device, m_whiteTex);
    vk::freeCommandBuffers(g_renderContext.device, m_commandPool, m_commandBuffers);
    vk::destroyRenderPass(g_renderContext.device, m_renderPass);
    vkDestroyCommandPool(g_renderContext.device.logical, m_commandPool, nullptr);
    vkDestroyDescriptorSetLayout(g_renderContext.device.logical, m_dsLayout, nullptr);
}

void Q3BspMap::Init()
{
    // regular faces are simple triangle lists, patches are drawn as triangle strips, so we need extra pipeline
    m_facesPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_patchPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    VK_VERIFY(vk::createRenderPass(g_renderContext.device, g_renderContext.swapChain, &m_renderPass));
    VK_VERIFY(vk::createCommandPool(g_renderContext.device, &m_commandPool));

    // build the swap chain
    g_renderContext.RecreateSwapChain(m_commandPool, m_renderPass);

    // stub missing texture used if original Quake assets are missing
    m_missingTex = TextureManager::GetInstance()->LoadTexture("res/missing.png", m_commandPool);

    // load textures
    LoadTextures();

    // load lightmaps
    LoadLightmaps();

    // create renderable faces and patches
    m_renderLeaves.reserve(leaves.size());

    for (const auto &l : leaves)
    {
        m_renderLeaves.push_back(Q3LeafRenderable());
        m_renderLeaves.back().visCluster = l.cluster;
        m_renderLeaves.back().firstFace  = l.leafFace;
        m_renderLeaves.back().numFaces   = l.n_leafFaces;

        // create a bounding box
        m_renderLeaves.back().boundingBoxVertices[0] = Math::Vector3f((float)l.mins.x, (float)l.mins.y, (float)l.mins.z);
        m_renderLeaves.back().boundingBoxVertices[1] = Math::Vector3f((float)l.mins.x, (float)l.mins.y, (float)l.maxs.z);
        m_renderLeaves.back().boundingBoxVertices[2] = Math::Vector3f((float)l.mins.x, (float)l.maxs.y, (float)l.mins.z);
        m_renderLeaves.back().boundingBoxVertices[3] = Math::Vector3f((float)l.mins.x, (float)l.maxs.y, (float)l.maxs.z);
        m_renderLeaves.back().boundingBoxVertices[4] = Math::Vector3f((float)l.maxs.x, (float)l.mins.y, (float)l.mins.z);
        m_renderLeaves.back().boundingBoxVertices[5] = Math::Vector3f((float)l.maxs.x, (float)l.mins.y, (float)l.maxs.z);
        m_renderLeaves.back().boundingBoxVertices[6] = Math::Vector3f((float)l.maxs.x, (float)l.maxs.y, (float)l.mins.z);
        m_renderLeaves.back().boundingBoxVertices[7] = Math::Vector3f((float)l.maxs.x, (float)l.maxs.y, (float)l.maxs.z);

        for (int i = 0; i < 8; ++i)
        {
            m_renderLeaves.back().boundingBoxVertices[i].m_x /= Q3BspMap::s_worldScale;
            m_renderLeaves.back().boundingBoxVertices[i].m_y /= Q3BspMap::s_worldScale;
            m_renderLeaves.back().boundingBoxVertices[i].m_z /= Q3BspMap::s_worldScale;
        }
    }

    m_renderFaces.reserve(faces.size());

    // create a common descriptor set layout and vertex buffer info
    m_vbInfo.bindingDescriptions.push_back(vk::getBindingDescription(sizeof(Q3BspVertexLump)));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inVertex, VK_FORMAT_R32G32B32_SFLOAT, 0));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inTexCoord, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3f)));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inTexCoordLightmap, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3f) + sizeof(vec2f)));
    CreateDescriptorSetLayout();

    // single shared uniform buffer
    VK_VERIFY(vk::createUniformBuffer(g_renderContext.device, sizeof(UniformBufferObject), &m_renderBuffers.uniformBuffer));

    int faceArrayIdx  = 0;
    int patchArrayIdx = 0;

    for (const auto &f : faces)
    {
        m_renderFaces.push_back(Q3FaceRenderable());

        //is it a patch?
        if (f.type == FaceTypePatch)
        {
            m_renderFaces.back().index = patchArrayIdx;
            CreatePatch(f);

            // generate Vulkan buffers for current patch
            CreateBuffersForPatch(patchArrayIdx);
            ++patchArrayIdx;
        }
        else
        {
            m_renderFaces.back().index = faceArrayIdx;

            // generate Vulkan buffers for current face
            CreateBuffersForFace(f, faceArrayIdx);
        }

        ++faceArrayIdx;
        m_renderFaces.back().type = f.type;
    }

    m_mapStats.totalVertices = vertices.size();
    m_mapStats.totalFaces    = faces.size();
    m_mapStats.totalPatches  = patchArrayIdx;

    RebuildPipelines();
    VK_VERIFY(vk::createCommandBuffers(g_renderContext.device, m_commandPool, m_commandBuffers, g_renderContext.frameBuffers.size()));

    // set the scale-down uniform
    m_ubo.worldScaleFactor = 1.f / Q3BspMap::s_worldScale;
}

void Q3BspMap::OnRender()
{
    // update uniform buffers
    m_ubo.ModelViewProjectionMatrix = g_renderContext.ModelViewProjectionMatrix;
    m_frustum.UpdatePlanes();

    void *data;
    vmaMapMemory(g_renderContext.device.allocator, m_renderBuffers.uniformBuffer.allocation, &data);
    memcpy(data, &m_ubo, sizeof(m_ubo));
    vmaUnmapMemory(g_renderContext.device.allocator, m_renderBuffers.uniformBuffer.allocation);

    // record new set of command buffers including only visible faces and patches
    RecordCommandBuffers();

    // render visible faces
    VK_VERIFY(g_renderContext.Submit(m_commandBuffers));
}

void Q3BspMap::OnWindowChanged()
{
    g_renderContext.RecreateSwapChain(m_commandPool, m_renderPass);
    RebuildPipelines();
}

// determine if a bsp cluster is visible from a given camera cluster
bool Q3BspMap::ClusterVisible(int cameraCluster, int testCluster) const
{
    if (!visData.vecs || (cameraCluster < 0)) {
        return true;
    }

    int idx = (cameraCluster * visData.sz_vecs) + (testCluster >> 3);

    return (visData.vecs[idx] & (1 << (testCluster & 7))) != 0;
}

// determine which bsp leaf camera resides in
int Q3BspMap::FindCameraLeaf(const Math::Vector3f &cameraPosition) const
{
    int leafIndex = 0;

    while (leafIndex >= 0)
    {
        // children.x - front node; children.y - back node
        if (PointPlanePos(planes[nodes[leafIndex].plane].normal.x,
                          planes[nodes[leafIndex].plane].normal.y,
                          planes[nodes[leafIndex].plane].normal.z,
                          planes[nodes[leafIndex].plane].dist, 
                          cameraPosition) == Math::PointInFrontOfPlane)
        {
            leafIndex = nodes[leafIndex].children.x;
        }
        else
        {
            leafIndex = nodes[leafIndex].children.y;
        }
    }

    return ~leafIndex;
}


//Calculate which faces to draw given a camera position & view frustum
void Q3BspMap::CalculateVisibleFaces(const Math::Vector3f &cameraPosition)
{
    m_visibleFaces.clear();
    m_visiblePatches.clear();

    //calculate the camera leaf
    int cameraLeaf    = FindCameraLeaf(cameraPosition * Q3BspMap::s_worldScale);
    int cameraCluster = m_renderLeaves[cameraLeaf].visCluster;

    //loop through the leaves
    for (const auto &rl : m_renderLeaves)
    {
        //if the leaf is not in the PVS - skip it
        if (!HasRenderFlag(Q3RenderSkipPVS) && !ClusterVisible(cameraCluster, rl.visCluster))
            continue;

        //if this leaf does not lie in the frustum - skip it
        if (!HasRenderFlag(Q3RenderSkipFC) && !m_frustum.BoxInFrustum(rl.boundingBoxVertices))
            continue;

        //loop through faces in this leaf and them to visibility set
        for (int j = 0; j < rl.numFaces; ++j)
        {
            int idx = leafFaces[rl.firstFace + j].face;
            Q3FaceRenderable *face = &m_renderFaces[leafFaces[rl.firstFace + j].face];

            if (HasRenderFlag(Q3RenderSkipMissingTex) && !m_textures[faces[idx].texture])
                continue;

            if ((face->type == FaceTypePolygon || face->type == FaceTypeMesh) &&
                std::find(m_visibleFaces.begin(), m_visibleFaces.end(), face) == m_visibleFaces.end())
            {
                m_visibleFaces.push_back(face);
            }

            if (face->type == FaceTypePatch &&
                std::find(m_visiblePatches.begin(), m_visiblePatches.end(), idx) == m_visiblePatches.end())
            {
                m_visiblePatches.push_back(idx);
            }
        }
    }

    m_mapStats.visibleFaces = m_visibleFaces.size();
    m_mapStats.visiblePatches = m_visiblePatches.size();
}

void Q3BspMap::ToggleRenderFlag(int flag)
{
    m_renderFlags ^= flag;
    bool set = HasRenderFlag(flag);

    switch (flag)
    {
    case Q3RenderShowWireframe:
        m_facesPipeline.mode = set ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        m_patchPipeline.mode = set ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        RebuildPipelines();
        break;
    case Q3RenderShowLightmaps:
        m_ubo.renderLightmaps = set ? 1 : 0;
        break;
    case Q3RenderUseLightmaps:
        m_ubo.useLightmaps = set ? 1 : 0;
        break;
    case Q3RenderAlphaTest:
        m_ubo.useAlphaTest = set ? 1 : 0;
        break;
    default:
        break;
    }
}

void Q3BspMap::LoadTextures()
{
    int numTextures = header.direntries[Textures].length / sizeof(Q3BspTextureLump);

    m_textures.resize(numTextures);

    // load the textures from file (determine wheter it's a jpg or tga)
    for (const auto &f : faces)
    {
        if (m_textures[f.texture])
            continue;

        std::string nameJPG = textures[f.texture].name;

        nameJPG.append(".jpg");

        m_textures[f.texture] = TextureManager::GetInstance()->LoadTexture(nameJPG.c_str(), m_commandPool);

        if (m_textures[f.texture] == nullptr)
        {
            std::string nameTGA = textures[f.texture].name;
            nameTGA.append(".tga");

            m_textures[f.texture] = TextureManager::GetInstance()->LoadTexture(nameTGA.c_str(), m_commandPool);

            if (m_textures[f.texture] == nullptr)
            {
                std::stringstream sstream;
                sstream << "Missing texture: " << nameTGA.c_str() << "\n";
                LOG_MESSAGE(sstream.str().c_str());
            }
        }
    }
}

void Q3BspMap::LoadLightmaps()
{
    m_lightmapTextures = new vk::Texture[lightMaps.size()];

    // optional: change gamma settings of the lightmaps (make them brighter)
    SetLightmapGamma(2.5f);

    // few GPUs support true 24bit textures, so we need to convert lightmaps to 32bit for Vulkan to work
    unsigned char rgba_lmap[128 * 128 * 4];
    for (size_t i = 0; i < lightMaps.size(); ++i)
    {
        memset(rgba_lmap, 255, 128 * 128 * 4);
        for (int j = 0, k = 0; k < 128 * 128 * 3; j += 4, k += 3)
            memcpy(rgba_lmap + j, lightMaps[i].map + k, 3);

        // Create texture from bsp lightmap data
        vk::createTexture(g_renderContext.device, m_commandPool, &m_lightmapTextures[i], rgba_lmap, 128, 128);
    }

    // Create white texture for if no lightmap specified
    unsigned char white[] = { 255, 255, 255, 255 };

    vk::createTexture(g_renderContext.device, m_commandPool, &m_whiteTex, white, 1, 1);
}

// tweak lightmap gamma settings
void Q3BspMap::SetLightmapGamma(float gamma)
{
    for (size_t i = 0; i < lightMaps.size(); ++i)
    {
        for (int j = 0; j < 128 * 128; ++j)
        {
            float r, g, b;

            r = lightMaps[i].map[j * 3 + 0];
            g = lightMaps[i].map[j * 3 + 1];
            b = lightMaps[i].map[j * 3 + 2];

            r *= gamma / 255.0f;
            g *= gamma / 255.0f;
            b *= gamma / 255.0f;

            float scale = 1.0f;
            float temp;
            if (r > 1.0f && (temp = (1.0f / r)) < scale) scale = temp;
            if (g > 1.0f && (temp = (1.0f / g)) < scale) scale = temp;
            if (b > 1.0f && (temp = (1.0f / b)) < scale) scale = temp;

            scale *= 255.0f;
            r *= scale;
            g *= scale;
            b *= scale;

            lightMaps[i].map[j * 3 + 0] = (unsigned char)r;
            lightMaps[i].map[j * 3 + 1] = (unsigned char)g;
            lightMaps[i].map[j * 3 + 2] = (unsigned char)b;
        }
    }
}

// create a Q3Bsp curved surface
void Q3BspMap::CreatePatch(const Q3BspFaceLump &f)
{
    Q3BspPatch *newPatch = new Q3BspPatch;

    newPatch->textureIdx  = f.texture;
    newPatch->lightmapIdx = f.lm_index;
    newPatch->width  = f.size.x;
    newPatch->height = f.size.y;

    int numPatchesWidth  = (newPatch->width  - 1) >> 1;
    int numPatchesHeight = (newPatch->height - 1) >> 1;

    newPatch->quadraticPatches.resize(numPatchesWidth*numPatchesHeight);

    // generate biquadratic patches (components that make the curved surface)
    for (int y = 0; y < numPatchesHeight; ++y)
    {
        for (int x = 0; x < numPatchesWidth; ++x)
        {
            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    int patchIdx = y * numPatchesWidth + x;
                    int cpIdx = row * 3 + col;
                    newPatch->quadraticPatches[patchIdx].controlPoints[cpIdx] = vertices[f.vertex +
                                                                                         (y * 2 * newPatch->width + x * 2) +
                                                                                         row * newPatch->width + col];
                }
            }

            newPatch->quadraticPatches[y * numPatchesWidth + x].Tesselate(Q3BspMap::s_tesselationLevel);
        }
    }

    m_patches.push_back(newPatch);
}

void Q3BspMap::RecordCommandBuffers()
{
    for (size_t i = 0; i < m_commandBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);
        LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Could not begin command buffer: " << result);

        VkClearValue clearColors[2];
        clearColors[0].color = { 0.f, 0.f, 0.f, 1.f };
        clearColors[1].depthStencil = { 1.0f, 0 };
        VkRenderPassBeginInfo renderBeginInfo = {};
        renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderBeginInfo.renderPass = m_renderPass.renderPass;
        renderBeginInfo.framebuffer = g_renderContext.frameBuffers[i];
        renderBeginInfo.renderArea.offset = { 0, 0 };
        renderBeginInfo.renderArea.extent = g_renderContext.swapChain.extent;
        renderBeginInfo.clearValueCount = 2;
        renderBeginInfo.pClearValues = clearColors;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // queue standard faces
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_facesPipeline.pipeline);

        for (auto &f : m_visibleFaces)
        {
            FaceBuffers &fb = m_renderBuffers.m_faceBuffers[f->index];

            VkBuffer vertexBuffers[] = { fb.vertexBuffer.buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
            // quake 3 bsp requires uint32 for index type - 16 is too small
            vkCmdBindIndexBuffer(m_commandBuffers[i], fb.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_facesPipeline.layout, 0, 1, &fb.descriptor.set, 0, nullptr);
            vkCmdDrawIndexed(m_commandBuffers[i], fb.indexCount, 1, 0, 0, 0);
        }

        // queue patches
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_patchPipeline.pipeline);

        for (auto &pi : m_visiblePatches)
        {
            for (auto &p : m_renderBuffers.m_patchBuffers[pi])
            {
                VkBuffer vertexBuffers[] = { p.vertexBuffer.buffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
                // quake 3 bsp requires uint32 for index type - 16 is too small
                vkCmdBindIndexBuffer(m_commandBuffers[i], p.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_patchPipeline.layout, 0, 1, &p.descriptor.set, 0, nullptr);
                vkCmdDrawIndexed(m_commandBuffers[i], p.indexCount, 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(m_commandBuffers[i]);

        result = vkEndCommandBuffer(m_commandBuffers[i]);
        LOG_MESSAGE_ASSERT(result == VK_SUCCESS, "Error recording command buffer: " << result);
    }
}

void Q3BspMap::RebuildPipelines()
{
    vkDeviceWaitIdle(g_renderContext.device.logical);
    vk::destroyPipeline(g_renderContext.device, m_facesPipeline);
    vk::destroyPipeline(g_renderContext.device, m_patchPipeline);

    // todo: pipeline derivatives https://github.com/SaschaWillems/Vulkan/blob/master/examples/pipelines/pipelines.cpp
    const char *shaders[] = { "res/Basic_vert.spv", "res/Basic_frag.spv" };
    VK_VERIFY(vk::createPipeline(g_renderContext.device, g_renderContext.swapChain, m_renderPass, m_dsLayout, &m_vbInfo, &m_facesPipeline, shaders));
    VK_VERIFY(vk::createPipeline(g_renderContext.device, g_renderContext.swapChain, m_renderPass, m_dsLayout, &m_vbInfo, &m_patchPipeline, shaders));
}

void Q3BspMap::CreateBuffersForFace(const Q3BspFaceLump &face, int idx)
{
    if (face.type == FaceTypeBillboard)
        return;

    auto &faceBuffer = m_renderBuffers.m_faceBuffers[idx];
    faceBuffer.descriptor.setLayout = m_dsLayout;
    faceBuffer.vertexCount = face.n_vertexes;
    faceBuffer.indexCount  = face.n_meshverts;

    // vertex buffer and index buffer with staging buffer
    vk::createVertexBuffer(g_renderContext.device, m_commandPool,
                           &(vertices[face.vertex].position), sizeof(Q3BspVertexLump) * face.n_vertexes, &faceBuffer.vertexBuffer);
     vk::createIndexBuffer(g_renderContext.device, m_commandPool,
                           &meshVertices[face.meshvert], sizeof(Q3BspMeshVertLump) * face.n_meshverts, &faceBuffer.indexBuffer);
    // check if both the texture and lightmap exist and if not - replace them with missing/white texture stubs
    const vk::Texture *colorTex = m_textures[faces[idx].texture] ? *m_textures[faces[idx].texture] : *m_missingTex;
    const vk::Texture &lmap = faces[idx].lm_index >= 0 ? m_lightmapTextures[faces[idx].lm_index] : m_whiteTex;

    const vk::Texture *textureSet[2] = { colorTex, &lmap };
    CreateDescriptor(textureSet, &faceBuffer.descriptor);
}

void Q3BspMap::CreateBuffersForPatch(int idx)
{
    int numPatches = m_patches[idx]->quadraticPatches.size();

    for (int i = 0; i < numPatches; i++)
    {
        // shorthand references so the code is easier to read
        auto *patch       = m_patches[idx];
        auto &biquadPatch = patch->quadraticPatches[i];
        auto &patchBuffer = m_renderBuffers.m_patchBuffers[idx];
        int numVerts  = biquadPatch.m_vertices.size();
        int tessLevel = biquadPatch.m_tesselationLevel;

        for (int row = 0; row < tessLevel; ++row)
        {
            FaceBuffers pb;
            pb.descriptor.setLayout = m_dsLayout;
            pb.vertexCount = numVerts;
            pb.indexCount  = 2 * (tessLevel + 1);

            // vertex buffer and index buffer with staging buffer
            vk::createVertexBuffer(g_renderContext.device, m_commandPool,
                                   &biquadPatch.m_vertices[0].position, sizeof(Q3BspVertexLump) * numVerts, &pb.vertexBuffer);
             vk::createIndexBuffer(g_renderContext.device, m_commandPool,
                                   &biquadPatch.m_indices[row * 2 * (tessLevel + 1)], sizeof(Q3BspMeshVertLump) * 2 * (tessLevel + 1), &pb.indexBuffer);
            // check if both the texture and lightmap exist and if not - replace them with missing/white texture stubs
            const vk::Texture *colorTex = m_textures[patch->textureIdx] ? *m_textures[patch->textureIdx] : *m_missingTex;
            const vk::Texture &lmap = patch->lightmapIdx >= 0 ? m_lightmapTextures[patch->lightmapIdx] : m_whiteTex;

            // create Vulkan descriptor
            const vk::Texture *textureSet[] = { colorTex, &lmap };
            CreateDescriptor(textureSet, &pb.descriptor);

            patchBuffer.emplace_back(pb);
        }
    }
}

void Q3BspMap::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding lightmapSamplerLayoutBinding = {};
    lightmapSamplerLayoutBinding.binding = 2;
    lightmapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightmapSamplerLayoutBinding.descriptorCount = 1;
    lightmapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightmapSamplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding, lightmapSamplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;

    VK_VERIFY(vkCreateDescriptorSetLayout(g_renderContext.device.logical, &layoutInfo, nullptr, &m_dsLayout));
}

void Q3BspMap::CreateDescriptor(const vk::Texture **textures, vk::Descriptor *descriptor)
{
    // create descriptor pool
    VkDescriptorPoolSize poolSizes[3];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    VK_VERIFY(vkCreateDescriptorPool(g_renderContext.device.logical, &poolInfo, nullptr, &descriptor->pool));

    // create descriptor set
    VK_VERIFY(vk::createDescriptorSet(g_renderContext.device, descriptor));
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.offset = 0;
    bufferInfo.buffer = m_renderBuffers.uniformBuffer.buffer;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textures[0]->imageView;
    imageInfo.sampler = textures[0]->sampler;

    VkDescriptorImageInfo lightmapInfo = {};
    lightmapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    lightmapInfo.imageView = textures[1]->imageView;
    lightmapInfo.sampler = textures[1]->sampler;

    VkWriteDescriptorSet descriptorWrites[3];
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptor->set;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;
    descriptorWrites[0].pNext = nullptr;
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptor->set;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = nullptr;
    descriptorWrites[1].pImageInfo = &imageInfo;
    descriptorWrites[1].pTexelBufferView = nullptr;
    descriptorWrites[1].pNext = nullptr;
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptor->set;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = nullptr;
    descriptorWrites[2].pImageInfo = &lightmapInfo;
    descriptorWrites[2].pTexelBufferView = nullptr;
    descriptorWrites[2].pNext = nullptr;

    vkUpdateDescriptorSets(g_renderContext.device.logical, 3, descriptorWrites, 0, nullptr);
}
