#include "renderer/TextureManager.hpp"
#include "Utils.hpp"
#ifdef __APPLE__
#include "apple/AppleUtils.hpp"
#endif

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
#if TARGET_OS_IPHONE
        GameTexture *newTex = new GameTexture((getResourcePath() + textureName).c_str());
#else
        GameTexture *newTex = new GameTexture(textureName);
#endif
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
