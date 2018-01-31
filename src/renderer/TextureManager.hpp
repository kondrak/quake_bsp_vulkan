#ifndef TEXTUREMANAGER_HPP
#define TEXTUREMANAGER_HPP

#include "renderer/GameTexture.hpp"
#include <map>

/*
 * Container class for loading/releasing textures
 */

class TextureManager
{
public:
    static TextureManager* GetInstance();

    void ReleaseTextures();
    GameTexture *LoadTexture(const char *textureName, const VkCommandPool &commandPool, bool filtering = true);
private:
    TextureManager() {}
    ~TextureManager();

    std::map<std::string, GameTexture *> m_textures;
};

#endif
