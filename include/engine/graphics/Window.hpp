#pragma once

#include "Error.hpp"

#include <util.hpp>

namespace engine::graphics
{
    struct Instance;

    struct Image : util::Factory<Image>
    {
        using handle_t = void*;

        Image(id_t id, id_t d, handle_t i, handle_t i_v);
        ~Image();

    private:
        friend class Instance;

        handle_t image, image_view;
        id_t device;
    };

    struct Swapchain : util::Factory<Swapchain>
    {
        using handle_t = void*;

        Swapchain(id_t id, uint32_t w, uint32_t h);
        ~Swapchain();

        Swapchain(Swapchain&&) = delete;
        Swapchain(const Swapchain&) = delete;

    private:
        friend class Instance;

        handle_t handle;

        std::pair<uint32_t, uint32_t> size;

        std::vector<id_t> images;
    };

    struct Window : util::Factory<Window>
    {
        using handle_t = void*;

        ~Window();

        /* Error checking */
        constexpr bool good()          const { return !_error.has_value(); }
        constexpr operator bool()      const { return good(); }
        constexpr const Error& error() const { return _error.value(); }

        constexpr void close() { _open = false; }
        constexpr bool isOpen() const { return _open; }

        void display() const;
        void pollEvents();

    private:
        friend class util::Universe;

        Window(id_t id, uint32_t w, uint32_t h, const std::string& title = "Window");

        bool _open;
        handle_t handle;
        id_t swapchain;
        std::optional<Error> _error;
    };
}