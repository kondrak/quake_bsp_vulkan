#include "renderer/Font.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/CameraDirector.hpp"
#include "renderer/GameTexture.hpp"
#include "renderer/TextureManager.hpp"
#include "renderer/vulkan/CmdBuffer.hpp"
#include "Utils.hpp"

extern RenderContext  g_renderContext;
extern CameraDirector g_cameraDirector;

static const int CHAR_WIDTH  = 8;
static const int CHAR_HEIGHT = 9;
static const float CHAR_SPACING = 1.5f;


Font::Font(const char *tex) : m_scale(1.f, 1.f), m_position(0.0f, 0.0f, 0.0f), m_color(1.f, 1.f, 1.f)
{
    // characters are rendered as alpha blended triangle strips with no culling
    m_pipeline.cullMode  = VK_CULL_MODE_NONE;
    m_pipeline.topology  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    m_pipeline.blendMode = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_pipeline.cache     = g_renderContext.pipelineCache;
    m_pipeline.depthTestEnable = VK_FALSE;

    // load font texture
    m_texture = TextureManager::GetInstance()->LoadTexture(tex, g_renderContext.commandPool, false);
    LOG_MESSAGE_ASSERT(m_texture, "Could not load font texture: " << tex);

    // setup vertex attributes
    m_vbInfo.bindingDescriptions.push_back(vk::getBindingDescription(sizeof(GlyphVertex)));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inVertex, VK_FORMAT_R32G32B32_SFLOAT, 0));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inTexCoord, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));
    m_vbInfo.attributeDescriptions.push_back(vk::getAttributeDescription(inColor, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 5));

    // create vertex buffer and Vulkan descriptor
    vk::createVertexBuffer(g_renderContext.device, g_renderContext.commandPool, &m_charBuffer, sizeof(Glyph) * MAX_CHARS, &m_vertexBuffer);
    CreateDescriptor(*m_texture, &m_descriptor);

    RebuildPipeline();
}

Font::~Font()
{
    vk::destroyPipeline(g_renderContext.device, m_pipeline);

    vkDestroyDescriptorSetLayout(g_renderContext.device.logical, m_descriptor.setLayout, nullptr);
    vkDestroyDescriptorPool(g_renderContext.device.logical, m_descriptor.pool, nullptr);
    vk::freeBuffer(g_renderContext.device, m_vertexBuffer);
}

void Font::RenderText(const std::string &text, float x, float y, float z, float r, float g, float b)
{
    RenderText(text, Math::Vector3f(x, y, z), Math::Vector3f(r, g, b));
}

void Font::RenderText(const std::string &text, float x, float y, float z)
{
    RenderText(text, Math::Vector3f(x, y, z), m_color);
}

void Font::RenderText(const std::string &text)
{
    RenderText(text, m_position, m_color);
}

void Font::RenderText(const std::string &text, const Math::Vector3f &position, const Math::Vector3f &color)
{
    Camera::CameraMode camMode = g_cameraDirector.GetActiveCamera()->GetMode();
    g_cameraDirector.GetActiveCamera()->SetMode(Camera::CAM_ORTHO);

    Math::Vector3f pos = position;

    LOG_MESSAGE_ASSERT(m_charCount + text.length() < MAX_CHARS, "Too many chars");
    m_charCount += (int)text.length();

    for (int i = 0; i < text.length(); i++)
    {
        int cu = text[i] - 32;

        if (cu >= 0 && cu < 32 * 3)
        {
            DrawChar(pos, CHAR_WIDTH, CHAR_HEIGHT, cu % 16 * CHAR_WIDTH, cu / 16 * (CHAR_HEIGHT + 1), i, color);
        }

        pos.m_x += m_scale.m_x * (CHAR_SPACING / g_renderContext.scrRatio) * CHAR_WIDTH / g_renderContext.height;
        m_mappedData++;
    }

    g_cameraDirector.GetActiveCamera()->SetMode(camMode);
}

void Font::RenderStart()
{
    // reset character counter and start mapping vertex buffer memory
    m_charCount = 0;
    vmaMapMemory(g_renderContext.device.allocator, m_vertexBuffer.allocation, (void**)&m_mappedData);
}

void Font::RenderFinish()
{
    vmaUnmapMemory(g_renderContext.device.allocator, m_vertexBuffer.allocation);

    // update command buffers with new characters
    Draw();
}

void Font::RebuildPipeline()
{
    vk::destroyPipeline(g_renderContext.device, m_pipeline);

    // todo: pipeline derivatives https://github.com/SaschaWillems/Vulkan/blob/master/examples/pipelines/pipelines.cpp
    const char *shaders[] = { "res/Font_vert.spv", "res/Font_frag.spv" };
    VK_VERIFY(vk::createPipeline(g_renderContext.device, g_renderContext.swapChain, g_renderContext.renderPass, m_descriptor.setLayout, &m_vbInfo, &m_pipeline, shaders));
}

void Font::DrawChar(const Math::Vector3f &pos, int w, int h, int uo, int vo, int offset, const Math::Vector3f &color)
{
    Math::Matrix4f texMatrix, mvMatrix;

    Math::Scale(mvMatrix, 2.f * w / g_renderContext.height, 2.f * h / g_renderContext.height);
    Math::Scale(mvMatrix, m_scale.m_x, m_scale.m_y);

    Math::Scale(texMatrix, 1.f / m_texture->Width(), -1.f / m_texture->Height());
    Math::Translate(texMatrix, (float)uo, (float)-vo);
    Math::Scale(texMatrix, (float)w, (float)h);

    Math::Vector3f verts[]{ { 0.f, 0.f, -1.f },{ 0.f, -1.f, -1.f },
                            { 1.f, 0.f, -1.f },{ 1.f, -1.f, -1.f } };
    Math::Vector3f uv[4];
    Math::Vector3f p(pos.m_x, -pos.m_y, pos.m_z);
    Math::Vector3f uvo(texMatrix[12], texMatrix[13], 0.f);
    GlyphVertex g;

    for (int i = 0; i < 4; i++)
    {
        uv[i] = texMatrix * verts[i] + uvo;
        verts[i] = g_cameraDirector.GetActiveCamera()->ProjectionMatrix() * mvMatrix * verts[i] + p;

        memcpy(g.pos, &verts[i], sizeof(g.pos));
        memcpy(g.uv, &uv[i], sizeof(g.uv));
        memcpy(g.color, &m_color, sizeof(g.color));
        memcpy(&m_mappedData->verts[i], &g, sizeof(GlyphVertex));
    }
}

void Font::Draw()
{
    vkCmdBindPipeline(g_renderContext.activeCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);

    // queue all pending characters
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(g_renderContext.activeCmdBuffer, 0, 1, &m_vertexBuffer.buffer, offsets);
    vkCmdBindDescriptorSets(g_renderContext.activeCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout, 0, 1, &m_descriptor.set, 0, nullptr);

    for (int j = 0; j < m_charCount; j++)
        vkCmdDraw(g_renderContext.activeCmdBuffer, 4, 1, j * 4, 0);
}

void Font::CreateDescriptor(const vk::Texture *texture, vk::Descriptor *descriptor)
{
    // create descriptor set layout
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = bindings;

    VK_VERIFY(vkCreateDescriptorSetLayout(g_renderContext.device.logical, &layoutInfo, nullptr, &descriptor->setLayout));

    // create descriptor pool
    VkDescriptorPoolSize poolSizes;
    poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSizes;
    poolInfo.maxSets = 1;

    VK_VERIFY(vkCreateDescriptorPool(g_renderContext.device.logical, &poolInfo, nullptr, &descriptor->pool));
    VK_VERIFY(vk::createDescriptorSet(g_renderContext.device, descriptor));

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->imageView;
    imageInfo.sampler = texture->sampler;

    VkWriteDescriptorSet descriptorWrites;
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = descriptor->set;
    descriptorWrites.dstBinding = 1;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = nullptr;
    descriptorWrites.pImageInfo = &imageInfo;
    descriptorWrites.pTexelBufferView = nullptr;
    descriptorWrites.pNext = nullptr;

    vkUpdateDescriptorSets(g_renderContext.device.logical, 1, &descriptorWrites, 0, nullptr);
}
