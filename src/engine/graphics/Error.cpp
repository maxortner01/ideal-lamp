#include <engine/graphics/Error.hpp>

namespace engine::graphics
{
    std::string operator*(Error e)
    {
        switch (e)
        {
        case Error::SDL_INIT_ERROR:   return "SDL failed to initialize video";
        case Error::SDL_WINDOW_ERROR: return "SDL window creation failed";
        case Error::SDL_VULKAN_ERROR: return "SDL failed to create Vulkan surface";
        case Error::VULKAN_INSTANCE_ERROR: return "Vulkan instance failed to create";
        case Error::VULKAN_PHYSICAL_NOT_FOUND: return "Vulkan can't find a compatible physical device";
        default: return "";
        }
    }
}