#include "Application.hpp"
#include "InputHandlers.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/CameraDirector.hpp"
#include "Utils.hpp"

// for simplicity, let's use globals
RenderContext  g_renderContext;
Application    g_application;
CameraDirector g_cameraDirector;
int g_fps = 1;

int main(int argc, char **argv)
{
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LOG_MESSAGE_ASSERT(false, "Failed to initialize SDL.");
        return 1;
    }

    if (!g_renderContext.Init("Quake BSP Viewer in Vulkan", 100, 100, 1024, 768))
    {
        LOG_MESSAGE_ASSERT(false, "Could not initialize render context!");
        SDL_Quit();
        return 1;
    }

    SDL_ShowCursor(SDL_DISABLE);
    g_application.OnStart(argc, argv);

    double now = 0, last = 0;
    int numFrames = 0;
    float time = 0.f;

    while (g_application.Running())
    {
        // handle key presses
        processEvents();

        now = SDL_GetTicks();
        float dt = float(now - last) / 1000.f;

        g_application.OnUpdate(dt);

        g_renderContext.ModelViewProjectionMatrix = g_cameraDirector.GetActiveCamera()->ViewMatrix() * g_cameraDirector.GetActiveCamera()->ProjectionMatrix();
        g_application.OnRender();

        last = now;
        time += dt;
        numFrames++;

        if (time > .25f)
        {
            g_fps = int(numFrames / time);
            numFrames = 0;
            time = 0.f;
        }
    }

    g_application.OnTerminate();
    g_renderContext.Destroy();
    SDL_Quit();

    return 0;
}
