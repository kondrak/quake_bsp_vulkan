#ifndef FONT_HPP
#define FONT_HPP

#include "renderer/RenderContext.hpp"
#include <string>

class GameTexture;
#define MAX_CHARS 300

class Font
{
public:
    Font(const char *texture);
    ~Font();
    void SetColor(const Math::Vector4f &color) { m_color = color; }
    void SetPosition(const Math::Vector3f &position) { m_position = position; }
    void SetScale(const Math::Vector2f &scale) { m_scale = scale; }
    void drawText(const std::string &text);
    void drawText(const std::string &text, float x, float y, float z, float r, float g, float b, float a);
    void drawText(const std::string &text, const Math::Vector3f &position, const Math::Vector4f &color = Math::Vector4f(1.f, 1.f, 1.f, 1.f));
    void drawText(const std::string &text, float x, float y, float z = -1.0f);
    void OnWindowChanged();
    void drawStart();
    void drawFinish();
private:
    struct GlyphData
    {
        float pos[3];
        float uv[2];
        float color[3];
    };

    struct Glyph
    {
        GlyphData verts[4];
    };

    void rebuildPipeline();
    void recordCommandBuffers();
    void renderAt(const Math::Vector3f &pos, int w, int h, int uo, int vo, int offset, const Math::Vector4f &color);
    void createDescriptor(const vk::Texture *texture, vk::Descriptor *descriptor);
    GameTexture*    m_texture = nullptr;
    Math::Vector2f  m_scale;
    Math::Vector3f  m_position;
    Math::Vector4f  m_color;

    // Vulkan thingamabobs
    vk::Pipeline   m_pipeline;
    vk::RenderPass m_renderPass;
    VkCommandPool  m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    vk::VertexBufferInfo vbInfo;

    vk::Buffer     m_vertexBuffer;
    vk::Descriptor m_descriptor;

    int    m_charCount = 0;
    Glyph  m_charBuffer[MAX_CHARS];
    Glyph *m_mappedData = nullptr;
};

#endif