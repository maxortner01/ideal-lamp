#include <engine/graphics/Window.hpp>
#include <engine/graphics/Instance.hpp>

#include <SDL3/SDL.h>
#include "UtilSDL.cpp"

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>

namespace engine::graphics
{
    Image::Image(id_t id, id_t d, handle_t i, handle_t i_v) :
        Factory(id),
        device(d),
        image(i),
        image_view(i_v)
    {   }

    Image::~Image()
    {   
        auto d = Device::get(device);
        if (image)
            vkDestroyImage(
                static_cast<VkDevice>(d->getHandle()), 
                static_cast<VkImage>(image), 
                nullptr
            );

        if (image_view)
            vkDestroyImageView(
                static_cast<VkDevice>(d->getHandle()), 
                static_cast<VkImageView>(image_view), 
                nullptr
            );
    }

    Swapchain::Swapchain(id_t id, uint32_t w, uint32_t h) :
        Factory(id)
    {
        Instance::get().buildSwapchain(this, w, h);
    }

    Swapchain::~Swapchain()
    {
        // Destroy swapchain

        for (const auto& id : images)
            Image::destroy(id);
    }

    Window::~Window()
    {
        if (handle)
        {
            Instance::destroy();
            SDL_DestroyWindow(static_cast<SDL_Window*>(handle));
            handle = nullptr;
        }
    }

    Window::Window(id_t id, uint32_t w, uint32_t h, const std::string& title) :
        Factory(id),
        handle(nullptr),
        _error(std::nullopt),
        _open(false)
    {
        const auto res = sdl::init_video();
        if (res)
        { _error = *res; return; }

        handle = static_cast<handle_t>(SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_VULKAN));
        if (!handle)
        { _error = Error::SDL_WINDOW_ERROR; return; }

        auto& instance = Instance::get();
        if (!instance.good())
        { _error = instance.error(); return; }

        instance.initialize(
            [&]() -> handle_t
            {
                VkSurfaceKHR surface;
                if (!SDL_Vulkan_CreateSurface(
                    static_cast<SDL_Window*>(handle), 
                    static_cast<VkInstance>(instance.getHandle()), 
                    nullptr, 
                    &surface
                )) { this->_error = Error::SDL_VULKAN_ERROR; return nullptr; }

                return static_cast<handle_t>(surface);
            }
        );
        if (!instance.good())
        { _error = instance.error(); return; }

        swapchain = Swapchain::make(w, h)->getID();
        addDependency(swapchain);

        _open = true;
    }

    void Window::display() const
    {
        SDL_UpdateWindowSurface(static_cast<SDL_Window*>(handle));
    }
    
    void Window::pollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                close();
        }
    }
}
