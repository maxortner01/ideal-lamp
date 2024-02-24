#include <SDL3/SDL.h>
#include <engine/graphics/Window.hpp>

namespace sdl
{
    std::optional<engine::graphics::Error>
    init_video()
    {
        using namespace engine::graphics;

        if (!SDL_WasInit(SDL_INIT_VIDEO))
            if (SDL_Init(SDL_INIT_VIDEO) < 0)
                return { Error::SDL_INIT_ERROR };

        return std::nullopt;
    }
}