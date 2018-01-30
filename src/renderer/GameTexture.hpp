#ifndef GAMETEXTURE_INCLUDED
#define GAMETEXTURE_INCLUDED

#include "renderer/vulkan/Image.hpp"

/*
 *  Generic texture with Vulkan image buffer
 */

class GameTexture
{
public:
    friend class TextureManager;

    const int Width()  const { return m_width; }
    const int Height() const { return m_height; }

    // implicit conversion to vk::Texture* for fast reference to Vulkan image
    operator const vk::Texture*() const { return &m_vkTexture; }
private:
    GameTexture(const char *filename);
    ~GameTexture();

    bool Load(const VkCommandPool &commandPool, bool filtering);

    int m_width;
    int m_height;
    int m_components;
    vk::Texture m_vkTexture;
    unsigned char *m_textureData;
};

#endif