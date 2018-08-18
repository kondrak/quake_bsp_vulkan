#include <SDL.h>
#include "Application.hpp"
#include "StringHelpers.hpp"
#include "ThreadProcessor.hpp"
#include "renderer/CameraDirector.hpp"
#include "renderer/RenderContext.hpp"
#include "q3bsp/Q3BspLoader.hpp"
#include "q3bsp/Q3BspStatsUI.hpp"

extern RenderContext  g_renderContext;
extern CameraDirector g_cameraDirector;
extern ThreadProcessor g_threadProcessor;

void Application::OnWindowResize(int newWidth, int newHeight)
{
    // fast window resizes return incorrect results from polled event - Vulkan surface query does it better
    Math::Vector2f windowSize = g_renderContext.WindowSize();
    // skip drawing if either dimension is zero
    m_noRedraw = !(windowSize.m_x > 0 && windowSize.m_y > 0);

    // window size changed - recreate the swapchain here since some drivers (Intel) can't properly invalidate it
    if (!m_noRedraw)
        g_renderContext.RecreateSwapChain();
}

void Application::OnWindowMinimized(bool minimized)
{
    m_noRedraw = minimized;

    if (minimized)
        g_threadProcessor.Wait();
}

void Application::OnStart(int argc, char **argv)
{
    Q3BspLoader loader;
    // assume the parameter with a string ".bsp" is the map we want to load
    for (int i = 1; i < argc; ++i)
    {
        if (!m_q3map && std::string(argv[i]).find(".bsp") != std::string::npos)
        {
            m_q3map = loader.Load(argv[i]);
        }

        if (!strcmp(argv[i], "-mt"))
        {
            m_multithreaded = true;
        }
    }

    // avoid excessive if-checks if no argv BSP was supplied and just display the UI message
    if (!m_q3map)
        m_q3map = new Q3BspMap(false);

    m_q3map->Init(m_multithreaded);
    m_q3map->ToggleRenderFlag(Q3RenderUseLightmaps);

    // try to locate the first info_player_deathmatch entity and place the camera there
    Math::Vector3f startPos;
    if (m_q3map->Valid())
        startPos = FindPlayerStart(static_cast<Q3BspMap *>(m_q3map)->entities.ents);

    g_cameraDirector.AddCamera(startPos / Q3BspMap::s_worldScale,
                               Math::Vector3f(0.f, 0.f, 1.f),
                               Math::Vector3f(1.f, 0.f, 0.f),
                               Math::Vector3f(0.f, 1.f, 0.f));

    // set to "clean" perspective matrix
    g_cameraDirector.GetActiveCamera()->SetMode(Camera::CAM_FPS);

    m_q3stats = new Q3StatsUI(m_q3map);
}

void Application::OnRender()
{
    if (m_noRedraw)
        return;

    // incompatible swapchain - skip this frame
    if (g_renderContext.RenderStart() == VK_ERROR_OUT_OF_DATE_KHR)
        return;

    // render the bsp
    g_cameraDirector.GetActiveCamera()->UpdateView();
    m_q3map->OnRender(m_multithreaded);

    // render map stats
    switch (m_debugRenderState)
    {
    case RenderMapStats:
        m_q3stats->OnRender();
        break;
    default:
        break;
    }

    // submit graphics queue and present it to screen
    VK_VERIFY(g_renderContext.Submit());

    // present!
    g_renderContext.Present();
}

void Application::OnUpdate(float dt)
{
    UpdateCamera(dt);

    // determine which faces are visible
    if (m_q3map->Valid() && !m_noRedraw)
        m_q3map->OnUpdate(m_multithreaded);
}

void Application::OnTerminate()
{
    vkDeviceWaitIdle(g_renderContext.device.logical);
    delete m_q3stats;
    delete m_q3map;
}

bool Application::KeyPressed(KeyCode key)
{
    // to be 100% no undefined state exists
    if (m_keyStates.find(key) == m_keyStates.end())
        m_keyStates[key] = false;

    return m_keyStates[key];
}

void Application::OnKeyPress(KeyCode key)
{
    SetKeyPressed(key, true);

    switch (key)
    {
    case KEY_F1:
        m_q3map->ToggleRenderFlag(Q3RenderShowWireframe);
        break;
    case KEY_F2:
        m_q3map->ToggleRenderFlag(Q3RenderShowLightmaps);
        break;
    case KEY_F3:
        m_q3map->ToggleRenderFlag(Q3RenderUseLightmaps);
        break;
    case KEY_F4:
        m_q3map->ToggleRenderFlag(Q3RenderAlphaTest);
        break;
    case KEY_F5:
        m_q3map->ToggleRenderFlag(Q3RenderSkipMissingTex);
        break;
    case KEY_F6:
        m_q3map->ToggleRenderFlag(Q3RenderSkipPVS);
        break;
    case KEY_F7:
        m_q3map->ToggleRenderFlag(Q3RenderSkipFC);
        break;
    case KEY_F8:
        m_q3map->ToggleRenderFlag(Q3Multisampling);
        g_renderContext.ToggleMSAA();
        m_q3map->RebuildPipeline();
        m_q3stats->RebuildPipeline();
        break;
    case KEY_TILDE:
        m_debugRenderState++;

        if (m_debugRenderState >= DebugRenderMax)
            m_debugRenderState = None;
        break;
    case KEY_ESC:
        Terminate();
        break;
    default:
        break;
    }
}

void Application::OnKeyRelease(KeyCode key)
{
    SetKeyPressed(key, false);
}

void Application::OnMouseMove(int x, int y)
{
    g_cameraDirector.GetActiveCamera()->OnMouseMove(x, y);
}

void Application::UpdateCamera(float dt)
{
    static const float movementSpeed = 8.f;

    if (KeyPressed(KEY_A))
        g_cameraDirector.GetActiveCamera()->Strafe(-movementSpeed * dt);

    if (KeyPressed(KEY_D))
        g_cameraDirector.GetActiveCamera()->Strafe(movementSpeed * dt);

    if (KeyPressed(KEY_W))
        g_cameraDirector.GetActiveCamera()->MoveForward(-movementSpeed * dt);

    if (KeyPressed(KEY_S))
        g_cameraDirector.GetActiveCamera()->MoveForward(movementSpeed * dt);

    // do the barrel roll!
    if (KeyPressed(KEY_Q))
        g_cameraDirector.GetActiveCamera()->rotateZ(2.f * dt);

    if (KeyPressed(KEY_E))
        g_cameraDirector.GetActiveCamera()->rotateZ(-2.f * dt);

    // move straight up/down
    if (KeyPressed(KEY_R))
        g_cameraDirector.GetActiveCamera()->MoveUpward(movementSpeed * dt);

    if (KeyPressed(KEY_F))
        g_cameraDirector.GetActiveCamera()->MoveUpward(-movementSpeed * dt);
}

Math::Vector3f Application::FindPlayerStart(const char *entities)
{
    std::string str(entities);
    Math::Vector3f result(0.0f, 0.0f, 4.0f); // some arbitrary position in case there's no info_player_deathmatch on map
    size_t pos = 0;

    std::string playerStartCoords("");

    while (pos != std::string::npos)
    {
        size_t posOpen = str.find("{", pos);
        size_t posClose = str.find("}", posOpen);

        if (posOpen != std::string::npos)
        {
            std::string entity = str.substr(posOpen + 1, posClose - posOpen - 1);
            if (FindEntityAttribute(entity, "info_player_deathmatch", "origin", playerStartCoords))
                break;
        }

        pos = posClose;
    }

    if (!playerStartCoords.empty())
    {
        auto coords = StringHelpers::tokenizeString(playerStartCoords.c_str(), ' ');

        result.m_x = std::stof(coords[0]);
        result.m_y = std::stof(coords[1]);
        result.m_z = std::stof(coords[2]);
    }

    return result;
}

bool Application::FindEntityAttribute(const std::string &entity, const char *entityName, const char *attribName, std::string &output)
{
    output.clear();
    std::string trimmedEntity = StringHelpers::trim(entity, '\n');
    std::string attribValueFinal("");
    std::string attribValueRead("");
    bool attribFound = false;

    auto tokens = StringHelpers::tokenizeString(trimmedEntity.c_str(), '\n');

    for (auto &t : tokens)
    {
        auto attribs = StringHelpers::tokenizeString(t.c_str(), ' ', 2);

        for (auto &a : attribs)
        {
            a = StringHelpers::trim(a, '"');
        }

        if (!strncmp(attribs[0].c_str(), attribName, strlen(attribs[0].c_str())))
        {
            attribValueRead = attribs[1];
        }

        if (!strncmp(attribs[1].c_str(), entityName, strlen(attribs[1].c_str())))
        {
            attribFound = true;
        }
    }

    if (attribFound)
        output = attribValueRead;

    return attribFound;
}