#include "Application.hpp"
#include "q3bsp/Q3BspMap.hpp"
#include "q3bsp/Q3BspStatsUI.hpp"
#include "renderer/RenderContext.hpp"
#include <sstream>

extern RenderContext g_renderContext;
extern Application   g_application;
extern int g_fps;

Q3StatsUI::Q3StatsUI(BspMap *map) : StatsUI(map)
{
    m_font = new Font("res/font.png");
    m_font->SetScale(Math::Vector2f(2.f, 2.f));
}

void Q3StatsUI::OnRender()
{
    m_font->RenderStart();

    // no map loaded or no cmdline parameter specified - display error message
    if (!m_map->Valid())
    {
        m_font->SetColor(Math::Vector3f(1.f, 0.f, 0.f));
        m_font->RenderText("Error loading BSP - missing/corrupted file or no file specified!", -0.7f, 0.0f, 0.f);
        m_font->RenderFinish();
        return;
    }

    static const float statsX   = -0.99f;
    static const float keysX    =  0.34f;
    static const float statsY   =  0.70f;
    static const float keysY    = -0.25f;
    static const float ySpacing =  0.05f;

    const BspStats &stats = m_map->GetMapStats();

    std::stringstream statsStream;
    statsStream.precision(5);
    statsStream << "FPS: " << g_fps << " (" << (g_fps > 0 ? (1000.f / g_fps) : 0) << "ms)";
    m_font->RenderText(statsStream.str(), statsX, statsY + 6 * ySpacing, 0.f);

    statsStream.str("");
    statsStream << "Total vertices: " << stats.totalVertices;
    m_font->RenderText(statsStream.str(), statsX, statsY, 0.f);

    statsStream.str("");
    statsStream << "Total faces: " << stats.totalFaces;
    m_font->RenderText(statsStream.str(), statsX, statsY - ySpacing, 0.f);

    statsStream.str("");
    statsStream << "Total patches: " << stats.totalPatches;
    m_font->RenderText(statsStream.str(), statsX, statsY - ySpacing * 2.f, 0.f);

    statsStream.str("");
    statsStream << "Rendered faces: " << stats.visibleFaces;
    m_font->RenderText(statsStream.str(), statsX, statsY - ySpacing * 3.f, 0.f);

    statsStream.str("");
    statsStream << "Rendered patches: " << stats.visiblePatches;
    m_font->RenderText(statsStream.str(), statsX, statsY - ySpacing * 4.f, 0.);

    m_font->SetColor(Math::Vector3f(1.f, 0.f, 0.f));
    m_font->RenderText(" ~ - toggle stats view", keysX, keysY, 0.f);

    if (m_map->HasRenderFlag(Q3RenderShowWireframe))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    else
        m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));
    m_font->RenderText("F1 - show wireframe", keysX, keysY - ySpacing, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderShowLightmaps))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F2 - show lightmaps", keysX, keysY - ySpacing * 2.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderUseLightmaps))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F3 - use lightmaps", keysX, keysY - ySpacing * 3.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderAlphaTest))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F4 - use alpha test", keysX, keysY - ySpacing * 4.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipMissingTex))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F5 - show missing textures", keysX, keysY - ySpacing * 5.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipPVS))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F6 - use PVS culling", keysX, keysY - ySpacing * 6.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipFC))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F7 - use frustum culling", keysX, keysY - ySpacing * 7.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3Multisampling))
        m_font->SetColor(Math::Vector3f(0.f, 1.f, 0.f));
    m_font->RenderText("F8 - multisampling (MSAAx" + std::to_string(g_renderContext.MSAASamples()) + ")", keysX, keysY - ySpacing * 8.f, 0.f);
    m_font->SetColor(Math::Vector3f(1.f, 1.f, 1.f));

    m_font->RenderFinish();
}

void Q3StatsUI::RebuildPipeline()
{
    m_font->RebuildPipeline();
}
