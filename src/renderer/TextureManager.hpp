#ifndef TEXTUREMANAGER_HPP
#define TEXTUREMANAGER_HPP

#include "renderer/GameTexture.hpp"
#include <map>

class TextureManager
{
public:
    static TextureManager* GetInstance();

    GameTexture *LoadTexture(const char *textureName, const VkCommandPool commandPool, bool filtering = true);
    void ReleaseTextures();
private:
    TextureManager() {}

    ~TextureManager();

    std::map<std::string, GameTexture *> m_textures;
};

#endif
