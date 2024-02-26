#include <engine/graphics/Window.hpp>
#include <engine/graphics/Instance.hpp>

#include <SDL3/SDL.h>
#include "UtilSDL.cpp"

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>

namespace engine::graphics
{
    Fence::Fence()
    {
        VkFenceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence fence;
        assert(vkCreateFence(
            static_cast<VkDevice>(Instance::get().getDevice()->getHandle()),
            &create_info,
            nullptr,
            &fence
        ) == VK_SUCCESS);
        handle = static_cast<handle_t>(fence);
    }

    Fence::~Fence()
    {
        vkDestroyFence(
            static_cast<VkDevice>(Instance::get().getDevice()->getHandle()),
            static_cast<VkFence>(handle),
            nullptr
        );
    }

    Semaphore::Semaphore()
    {
        VkSemaphoreCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        assert(vkCreateSemaphore(
            static_cast<VkDevice>(Instance::get().getDevice()->getHandle()),
            &create_info,
            nullptr,
            &semaphore
        ) == VK_SUCCESS);
        handle = static_cast<handle_t>(semaphore);
    }

    Semaphore::~Semaphore()
    {
        vkDestroySemaphore(
            static_cast<VkDevice>(Instance::get().getDevice()->getHandle()),
            static_cast<VkSemaphore>(handle),
            nullptr
        );
    }

    Frame::Frame(id_t id) :
        Factory(id)
    {   
        const auto device = Instance::get().getDevice();

        // Create Command Pool
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = device->getGraphicsQueue().index;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool pool;
        assert(vkCreateCommandPool(
            static_cast<VkDevice>(device->getHandle()),
            &create_info,
            nullptr,
            &pool
        ) == VK_SUCCESS);
        command.pool = static_cast<handle_t>(pool);

        // Create command buffer
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = pool;
        alloc_info.commandBufferCount = 1;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer buffer;
        assert(vkAllocateCommandBuffers(
            static_cast<VkDevice>(device->getHandle()),
            &alloc_info,
            &buffer
        ) == VK_SUCCESS);
        command.buffer = static_cast<handle_t>(buffer);
    }

    Frame::~Frame()
    {
        const VkDevice device = static_cast<VkDevice>(Instance::get().getDevice()->getHandle());

        vkDeviceWaitIdle(device);
        vkDestroyCommandPool(
            device,
            static_cast<VkCommandPool>(command.pool),
            nullptr
        );
    }

    Image::Image(id_t id, handle_t i, handle_t i_v) :
        Factory(id),
        image(i),
        image_view(i_v)
    {   }

    Image::~Image()
    {   
        auto device = Instance::get().getDevice()->getHandle();
        if (image)
            vkDestroyImage(
                static_cast<VkDevice>(device), 
                static_cast<VkImage>(image), 
                nullptr
            );

        if (image_view)
            vkDestroyImageView(
                static_cast<VkDevice>(device), 
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
        vkDestroySwapchainKHR(
            static_cast<VkDevice>(Instance::get().getDevice()->getHandle()), 
            static_cast<VkSwapchainKHR>(handle), 
            nullptr
        );

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
        _open(false),
        frame_number(0)
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

        const auto FRAME_COUNT = 2;
        frames.reserve(FRAME_COUNT);
        for (uint32_t i = 0; i < FRAME_COUNT; i++)
        {
            const auto id = Frame::make()->getID();
            frames.push_back(id);
            addDependency(id);
        }

        _open = true;
    }

    void Window::display()
    {
        const auto device = static_cast<VkDevice>(Instance::get().getDevice()->getHandle());
        auto frame = currentFrame();

        const auto fence = static_cast<VkFence>(frame->renderFence.getHandle());
        assert(vkWaitForFences(
            device,
            1,
            &fence,
            true,
            1000000000
        ) == VK_SUCCESS);
        assert(vkResetFences(device, 1, &fence) == VK_SUCCESS);

        uint32_t swapchain_index;
        const auto swapchainSemaphore = static_cast<VkSemaphore>(frame->swapchainSemaphore.getHandle());
        assert(vkAcquireNextImageKHR(
            device, 
            static_cast<VkSwapchainKHR>(Swapchain::get(swapchain)->getHandle()),
            1000000000,
            swapchainSemaphore,
            nullptr,
            &swapchain_index
        ) == VK_SUCCESS);

        // Command buffer stuff
        const auto command_buffer = static_cast<VkCommandBuffer>(frame->command.buffer);
        assert(vkResetCommandBuffer(command_buffer, 0) == VK_SUCCESS);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        assert(vkBeginCommandBuffer(command_buffer, &begin_info) == VK_SUCCESS);

        const auto image_subresource_range = []
        (VkImageAspectFlags aspectMask)
        {
            VkImageSubresourceRange subImage {};
            subImage.aspectMask = aspectMask;
            subImage.baseMipLevel = 0;
            subImage.levelCount = VK_REMAINING_MIP_LEVELS;
            subImage.baseArrayLayer = 0;
            subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

            return subImage;
        };

        const auto transition_image = [image_subresource_range]
        (VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
        {
            VkImageMemoryBarrier2 imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            imageBarrier.pNext = nullptr;

            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

            imageBarrier.oldLayout = currentLayout;
            imageBarrier.newLayout = newLayout;

            VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange = image_subresource_range(aspectMask);
            imageBarrier.image = image;

            VkDependencyInfo depInfo = {};
            depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depInfo.pNext = nullptr;

            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &imageBarrier;

            vkCmdPipelineBarrier2(cmd, &depInfo);
        };

        // Make the image writable
        const auto image = static_cast<VkImage>(Swapchain::get(swapchain)->getImage(swapchain_index)->getImageHandle());
        transition_image(command_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        //make a clear-color from frame number. This will flash with a 120 frame period.
        VkClearColorValue clearValue;
        float flash = abs(sin(frame_number / 120.f));
        clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

        const auto clearRange = image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

        //clear image
        vkCmdClearColorImage(command_buffer, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

        //make the swapchain image into presentable mode
        transition_image(command_buffer, image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        //finalize the command buffer (we can no longer add commands, but it can now be executed)
        assert(vkEndCommandBuffer(command_buffer) == VK_SUCCESS);

        frame_number++;
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
