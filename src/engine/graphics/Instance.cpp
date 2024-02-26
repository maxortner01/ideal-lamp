#include <engine/graphics/Instance.hpp>
#include <engine/graphics/Window.hpp>

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

namespace engine::graphics
{
    Device::Device(id_t id, handle_t h) :
        Factory(id),
        handle(h)
    {   }

    Device::~Device()
    {
        if (handle)
            vkDestroyDevice(static_cast<VkDevice>(handle), nullptr);
    }

    Instance::~Instance()
    {   
        if (device)
        {
            Device::destroy(device);
            device = 0;
        }

        if (surface && handle)
        {
            vkDestroySurfaceKHR(
                static_cast<VkInstance>(handle),
                static_cast<VkSurfaceKHR>(surface),
                nullptr
            );
            surface = 0;
        }
        
        if (handle)
        {
            vkDestroyInstance(static_cast<VkInstance>(handle), nullptr);
            handle = 0;
        }
    }

    Instance::Instance() :
        handle(nullptr),
        surface(nullptr),
        _error(std::nullopt)
    {   }
    
    void Instance::initialize(std::function<handle_t()> surface_func)
    {
        assert(!initialized());

        vkb::InstanceBuilder builder;
        auto res = builder.request_validation_layers()
                        .use_default_debug_messenger()
                        .set_minimum_instance_version(1, 2)
                        .build ();
        if (!res) { _error = Error::VULKAN_INSTANCE_ERROR; return; } 

        handle = static_cast<handle_t>(res->instance);
        debug  = static_cast<handle_t>(res->debug_messenger);
        
        surface = surface_func();

        // We require vulkan>1.3
        VkPhysicalDeviceVulkan13Features features{};
        features.dynamicRendering = true;
        features.synchronization2 = true;
        
        VkPhysicalDeviceVulkan12Features features12{};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        vkb::PhysicalDeviceSelector selector { *res };
        auto phys_res = selector
            .set_surface(static_cast<VkSurfaceKHR>(surface))
            .set_minimum_version(1, 2)
            .set_required_features_12(features12)
            .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
            .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
            .select();
        if (!phys_res) { _error = Error::VULKAN_PHYSICAL_NOT_FOUND; return; }
        physical_device = static_cast<handle_t>(phys_res->physical_device);

        vkb::DeviceBuilder device_b { *phys_res };
        auto dev_res = device_b.build();

        auto vk_device = Device::make(static_cast<handle_t>(dev_res->device));
        device = vk_device->getID();
        vk_device->graphics.queue = *dev_res->get_queue(vkb::QueueType::graphics);
        vk_device->graphics.index = *dev_res->get_queue_index(vkb::QueueType::graphics);
    }

    void Instance::buildSwapchain(Swapchain* swapchain, uint32_t w, uint32_t h) const
    {
        assert(initialized());

        vkb::SwapchainBuilder builder { 
            static_cast<VkPhysicalDevice>(physical_device),
            static_cast<VkDevice>(Device::get(device)->getHandle()),
            static_cast<VkSurfaceKHR>(surface)
        };

        const auto format = VK_FORMAT_B8G8R8A8_UNORM;

        auto swapchain_res = builder
            .set_desired_format({ format, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(w, h)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build();
        assert(swapchain_res);

        swapchain->size.first  = swapchain_res->extent.width;
        swapchain->size.second = swapchain_res->extent.height;
        swapchain->handle = static_cast<Swapchain::handle_t>(swapchain_res->swapchain);

        const auto images      = swapchain_res->get_images();
        const auto image_views = swapchain_res->get_image_views();
        assert(images && image_views);

        swapchain->images.reserve(images->size());
        for (uint32_t i = 0; i < images->size(); i++)
            swapchain->images.push_back(
                Image::make(
                    nullptr, 
                    static_cast<handle_t>(image_views->at(i))
                )->getID()
            );
    }
}
