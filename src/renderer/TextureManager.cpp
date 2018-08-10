#include "renderer/TextureManager.hpp"
#include "Utils.hpp"

TextureManager* TextureManager::GetInstance()
{
    static TextureManager instance;

    return &instance;
}

TextureManager::~TextureManager()
{
    ReleaseTextures();
}

void TextureManager::ReleaseTextures()
{
    for (std::map<std::string, GameTexture*>::iterator it = m_textures.begin(); it != m_textures.end(); ++it)
    {
        delete it->second;
    }

    m_textures.clear();
}

GameTexture *TextureManager::LoadTexture(const char *textureName, bool filtering)
{
    if (m_textures.count(textureName) == 0)
    {
        LOG_MESSAGE("[TextureManager] Loading texture: " << textureName);
        GameTexture *newTex = new GameTexture(textureName);

        // failed to load texture/file doesn't exist
        if (!newTex->Load(filtering))
        {
            delete newTex;
            return nullptr;
        }

        m_textures[textureName] = newTex;
    }

    return m_textures[textureName];
}
