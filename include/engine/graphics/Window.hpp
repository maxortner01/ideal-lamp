#pragma once

#include "Error.hpp"

#include <util.hpp>

namespace engine::graphics
{
    struct Instance;

    struct Fence
    {
        using handle_t = void*;

        Fence();
        ~Fence();

        Fence(Fence&&) = delete;
        Fence(const Fence&) = delete;

        const handle_t& getHandle() const { return handle; }

    private:
        handle_t handle;
    };

    struct Semaphore
    {
        using handle_t = void*;

        Semaphore();
        ~Semaphore();

        Semaphore(Semaphore&&) = delete;
        Semaphore(const Semaphore&) = delete;

        const handle_t& getHandle() const { return handle; }

    private:
        handle_t handle;
    };

    struct Frame : util::Factory<Frame>
    {
        using handle_t = void*;

        Frame(id_t id);
        ~Frame();

        struct Command
        {
            handle_t pool, buffer;
        } command;

        Semaphore swapchainSemaphore, renderSemaphore;
        Fence renderFence;
    };

    struct Image : util::Factory<Image>
    {
        using handle_t = void*;

        Image(id_t id, handle_t i, handle_t i_v);
        ~Image();

        const handle_t& getImageHandle() const { return image; }
        const handle_t& getImageViewHandle() const { return image_view; }

    private:
        friend class Instance;

        handle_t image, image_view;
    };

    struct Swapchain : util::Factory<Swapchain>
    {
        using handle_t = void*;

        Swapchain(id_t id, uint32_t w, uint32_t h);
        ~Swapchain();

        Swapchain(Swapchain&&) = delete;
        Swapchain(const Swapchain&) = delete;

        const handle_t& getHandle() const { return handle; }
        const std::shared_ptr<Image> getImage(const uint32_t& index) { return Image::get(images[index]); }

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

        void display();
        void pollEvents();

        std::shared_ptr<Frame> currentFrame() const { return Frame::get(frames[frame_number % frames.size()]); }

    private:
        friend class util::Universe;

        Window(id_t id, uint32_t w, uint32_t h, const std::string& title = "Window");

        bool _open;
        handle_t handle;
        
        id_t swapchain;
        std::vector<id_t> frames;
        uint32_t frame_number;

        std::optional<Error> _error;
    };
}