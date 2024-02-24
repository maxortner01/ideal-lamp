#pragma once

#include <expected>
#include <string>

namespace engine::graphics
{
    enum class Error
    {
        SDL_INIT_ERROR,
        SDL_WINDOW_ERROR,
        SDL_VULKAN_ERROR,

        VULKAN_INSTANCE_ERROR,
        VULKAN_PHYSICAL_NOT_FOUND
    };

    std::string operator*(Error e);

    template<typename T>
    using result = std::expected<T, Error>;
}