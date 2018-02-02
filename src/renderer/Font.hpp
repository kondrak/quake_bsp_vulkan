#ifndef FONT_HPP
#define FONT_HPP

#include "renderer/RenderContext.hpp"
#include <string>

class GameTexture;

/*
 * Basic bitmap font
 */

class Font
{
public:
    Font(const char *texture);
    ~Font();

    void SetColor(const Math::Vector3f &color) { m_color = color; }
    void SetPosition(const Math::Vector3f &position) { m_position = position; }
    void SetScale(const Math::Vector2f &scale) { m_scale = scale; }
    void RenderText(const std::string &text);
    void RenderText(const std::string &text, float x, float y, float z, float r, float g, float b);
    void RenderText(const std::string &text, const Math::Vector3f &position, const Math::Vector3f &color);
    void RenderText(const std::string &text, float x, float y, float z = -1.0f);
    void RenderStart();
    void RenderFinish();
    void RebuildPipeline();
private:
    static const int MAX_CHARS = 300;

    // vertex data for character
    struct GlyphVertex
    {
        float pos[3];
        float uv[2];
        float color[3];
    };

    // single character
    struct Glyph
    {
        GlyphVertex verts[4];
    };

    void RecordCommandBuffers();
    void CreateDescriptor(const vk::Texture *texture, vk::Descriptor *descriptor);
    // single character draw
    void DrawChar(const Math::Vector3f &pos, int w, int h, int uo, int vo, int offset, const Math::Vector3f &color);

    // handle to font texture
    GameTexture*    m_texture = nullptr;
    Math::Vector2f  m_scale;
    Math::Vector3f  m_position;
    Math::Vector3f  m_color;

    // Vulkan buffers
    vk::Pipeline   m_pipeline;
    vk::RenderPass m_renderPass;
    VkCommandPool  m_commandPool;
    vk::CmdBufferList m_commandBuffers;

    vk::VertexBufferInfo m_vbInfo;

    vk::Buffer     m_vertexBuffer;
    vk::Descriptor m_descriptor;

    int    m_charCount = 0;         // number of characters currently queued for drawing
    Glyph  m_charBuffer[MAX_CHARS]; // character data for vertex buffer
    Glyph *m_mappedData = nullptr;  // pointer to currently mapped Vulkan data
};

#endif