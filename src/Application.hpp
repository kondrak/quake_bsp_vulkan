#ifndef APPLICATION_INCLUDED
#define APPLICATION_INCLUDED

#include <map>
#include "InputHandlers.hpp"
#include "Math.hpp"

class BspMap;
class StatsUI;

/*
 * main application
 */

class Application
{
public:
    void OnWindowResize(int newWidth, int newHeight);
    void OnWindowMinimized(bool minimized);

    void OnStart(int argc, char **argv);
    void OnRender();
    void OnUpdate(float dt);
    void OnTerminate();

    inline bool Running() const { return m_running; }
    inline void Terminate() { m_running = false; }

    bool KeyPressed(KeyCode key);
    void OnKeyPress(KeyCode key);
    void OnKeyRelease(KeyCode key);
    void OnMouseMove(int x, int y);
private:
    enum DebugRender : uint8_t
    {
        None = 0,
        RenderMapStats,
        DebugRenderMax
    };

    void UpdateCamera(float dt);
    inline void SetKeyPressed(KeyCode key, bool pressed) { m_keyStates[key] = pressed; }

    // helper functions for parsing Quake entities
    Math::Vector3f FindPlayerStart(const char *entities);
    bool FindEntityAttribute(const std::string &entity, const char *entityName, const char *attribName, std::string &output);

    bool m_running     = true;    // application is running
    bool m_noRedraw    = false;   //  do not perform window redraw
    BspMap  *m_q3map   = nullptr; // loaded map
    StatsUI *m_q3stats = nullptr; // map stats UI
    uint8_t  m_debugRenderState = RenderMapStats;

    std::map< KeyCode, bool > m_keyStates;
};

#endif
