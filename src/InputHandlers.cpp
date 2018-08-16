#include "InputHandlers.hpp"
#include "Application.hpp"
#include "renderer/RenderContext.hpp"

extern Application g_application;
extern RenderContext g_renderContext;

KeyCode SDLKeyToKeyCode(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_1:
        return KEY_1;
    case SDLK_2:
        return KEY_2;
    case SDLK_3:
        return KEY_3;
    case SDLK_4:
        return KEY_4;
    case SDLK_5:
        return KEY_5;
    case SDLK_6:
        return KEY_6;
    case SDLK_7:
        return KEY_7;
    case SDLK_8:
        return KEY_8;
    case SDLK_9:
        return KEY_9;
    case SDLK_0:
        return KEY_0;
    case SDLK_a:
        return KEY_A;
    case SDLK_b:
        return KEY_B;
    case SDLK_c:
        return KEY_C;
    case SDLK_d:
        return KEY_D;
    case SDLK_e:
        return KEY_E;
    case SDLK_f:
        return KEY_F;
    case SDLK_g:
        return KEY_G;
    case SDLK_h:
        return KEY_H;
    case SDLK_i:
        return KEY_I;
    case SDLK_j:
        return KEY_J;
    case SDLK_k:
        return KEY_K;
    case SDLK_l:
        return KEY_L;
    case SDLK_m:
        return KEY_M;
    case SDLK_n:
        return KEY_N;
    case SDLK_o:
        return KEY_O;
    case SDLK_p:
        return KEY_P;
    case SDLK_q:
        return KEY_Q;
    case SDLK_r:
        return KEY_R;
    case SDLK_s:
        return KEY_S;
    case SDLK_t:
        return KEY_T;
    case SDLK_u:
        return KEY_U;
    case SDLK_v:
        return KEY_V;
    case SDLK_w:
        return KEY_W;
    case SDLK_x:
        return KEY_X;
    case SDLK_y:
        return KEY_Y;
    case SDLK_z:
        return KEY_Z;
    case SDLK_ESCAPE:
        return KEY_ESC;
    case SDLK_F1:
        return KEY_F1;
    case SDLK_F2:
        return KEY_F2;
    case SDLK_F3:
        return KEY_F3;
    case SDLK_F4:
        return KEY_F4;
    case SDLK_F5:
        return KEY_F5;
    case SDLK_F6:
        return KEY_F6;
    case SDLK_F7:
        return KEY_F7;
    case SDLK_F8:
        return KEY_F8;
    case SDLK_F9:
        return KEY_F9;
    case SDLK_F10:
        return KEY_F10;
    case SDLK_F11:
        return KEY_F11;
    case SDLK_F12:
        return KEY_F12;
    case SDLK_SPACE:
        return KEY_SPACE;
    case SDLK_BACKQUOTE:
        return KEY_TILDE;
    }

    return KEY_NULL;
}


void processEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            // alt + f4 handling
            if (event.key.keysym.sym == SDLK_F4 && (event.key.keysym.mod & KMOD_ALT))
            {
                g_application.Terminate();
                break;
            }
            // toggle fullscreen
            if (((event.key.keysym.sym == SDLK_RETURN) || (event.key.keysym.sym == SDLK_KP_ENTER)) && (event.key.keysym.mod & KMOD_ALT))
            {
                SDL_DisplayMode dMode;
                bool isFullScreen = (SDL_GetWindowFlags(g_renderContext.window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                SDL_SetWindowFullscreen(g_renderContext.window, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                SDL_GetCurrentDisplayMode(0, &dMode);
                g_renderContext.width  = dMode.w;
                g_renderContext.height = dMode.h;
                g_application.OnWindowResize(dMode.w, dMode.h);
                break;
            }
            g_application.OnKeyPress(SDLKeyToKeyCode(event.key.keysym.sym));
            break;
        case SDL_KEYUP:
            g_application.OnKeyRelease(SDLKeyToKeyCode(event.key.keysym.sym));
            break;
        case SDL_MOUSEMOTION:
            g_application.OnMouseMove(event.motion.x, event.motion.y);
            break;
        case SDL_QUIT:
            g_application.Terminate();
            break;
            // window events
        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                g_application.OnWindowResize(event.window.data1, event.window.data2);
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                g_application.OnWindowMinimized(true);
                break;
            case SDL_WINDOWEVENT_RESTORED:
                g_application.OnWindowMinimized(false);
                break;
            }
            break;
        }
    }
}
