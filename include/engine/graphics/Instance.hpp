#pragma once

#include <memory>
#include <functional>

#include "Error.hpp"
#include "Window.hpp"

namespace engine::graphics
{
    struct Swapchain;

    struct Device : util::Factory<Device>
    {
        using handle_t = void*;

        Device(id_t id, handle_t h);
        ~Device();

        Device(Device&&) = delete;
        Device(const Device&) = delete;
        
        const handle_t& getHandle() const { return handle; }

    private:
        handle_t handle;
    };

    struct Instance
    {
        using handle_t = void*;

        ~Instance();
        Instance(Instance&&) = delete;
        Instance(const Instance&) = delete;

        /* Singleton Behavior */
        static Instance& 
        get()
        {
            if (!instance)
                instance = new Instance();
            return *instance;
        }

        static void
        destroy()
        {
            if (instance)
            {
                delete instance;
                instance = nullptr;
            }
        }

        /* Instance Methods */
        const handle_t& getHandle()    const { return handle; }
        constexpr bool  good()         const { return !_error.has_value(); }
        constexpr const Error& error() const { return _error.value(); }

        void initialize(std::function<handle_t()> surface_func);
        bool initialized() const { return (handle != nullptr); }

        void buildSwapchain(Swapchain* swapchain, uint32_t w, uint32_t h) const;

    private:
        handle_t handle, surface, debug, physical_device;
        std::optional<Error> _error;

        id_t device = 0;

        inline static Instance* instance = nullptr;

        Instance();
    };
}