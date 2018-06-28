#include "renderer/RenderContext.hpp"
#include "renderer/GameTexture.hpp"
#include "stb_image/stb_image.h"
#include <algorithm>

extern RenderContext  g_renderContext;

GameTexture::GameTexture(const char *filename)
{
    VkFormatProperties fp = {};
    vkGetPhysicalDeviceFormatProperties(g_renderContext.device.physical, VK_FORMAT_R8G8B8_UNORM, &fp);

    // force rgba if rgb format is not supported by the device
    if (fp.optimalTilingFeatures > 0)
    {
        m_textureData = stbi_load(filename, &m_width, &m_height, &m_components, STBI_default);
    }
    else
    {
        m_textureData = stbi_load(filename, &m_width, &m_height, &m_components, STBI_rgb_alpha);
        m_components = 4;
    }
}

GameTexture::~GameTexture()
{
    if (m_textureData != nullptr)
        stbi_image_free(m_textureData);

    vk::releaseTexture(g_renderContext.device, m_vkTexture);
}

bool GameTexture::Load(const VkCommandPool &commandPool, bool filtering)
{
    // texture already loaded or doesn't exist
    if (!m_textureData)
        return false;

    if (!filtering)
    {
        m_vkTexture.minFilter = VK_FILTER_NEAREST;
        m_vkTexture.magFilter = VK_FILTER_NEAREST;
    }

    m_vkTexture.format = m_components == 3 ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    // calculate number of mipmaps to generate for given texture dimensions
    m_vkTexture.mipLevels = (uint32_t)std::floor(std::log2(std::max(m_width, m_height))) + 1;

    vk::createTexture(g_renderContext.device, commandPool, &m_vkTexture, m_textureData, m_width, m_height);

    stbi_image_free(m_textureData);
    m_textureData = nullptr;

    return true;
}
