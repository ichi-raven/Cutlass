#include <Device.hpp>

#include <iostream>
#include <vector>
#include <array>

namespace Cutlass
{
    #define GetInstanceProcAddr(FuncName) \
    m##FuncName = reinterpret_cast<PFN_##FuncName>(vkGetInstanceProcAddr(mInstance, #FuncName))

    static VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objactTypes,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char *pLayerPrefix,
        const char *pMessage,
        void *pUserData)
    {
        VkBool32 ret = VK_FALSE;
        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ||
            flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            ret = VK_TRUE;
        }

        if (pLayerPrefix)
        {
            std::cout << "[" << pLayerPrefix << "] ";
        }
        std::cout << pMessage << std::endl;

        return ret;
    }


    Device::Device()
    {

    }

    void Device::init()
    {

    }

    void Device::checkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cout << "error : " << result << "\n";
            exit(1);
        }
    }

    void Device::initializeInstance(const char* appName)
    {
        std::vector<const char*> extensions;
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName;
        appInfo.pEngineName = appName;
        appInfo.apiVersion = VK_API_VERSION_1_1;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        // 拡張情報の取得.
        std::vector<VkExtensionProperties> props;
        {
          uint32_t count = 0;
          vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
          props.resize(count);
          vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

          for (const auto& v : props)
          {
            extensions.push_back(v.extensionName);
          }
        }

        VkInstanceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; 
        ci.enabledExtensionCount = uint32_t(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.pApplicationInfo = &appInfo;
        #define _DEBUG
        #ifdef _DEBUG
        // デバッグビルド時には検証レイヤーを有効化
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        ci.enabledLayerCount = 1;
        ci.ppEnabledLayerNames = layers;
        #endif

        // インスタンス生成
        auto result = vkCreateInstance(&ci, nullptr, &mInstance);
        checkResult(result);
    }

    void Device::selectPhysicalDevice()
    {
        uint32_t devCount = 0;
        vkEnumeratePhysicalDevices(mInstance, &devCount, nullptr);
        std::vector<VkPhysicalDevice> physDevs(devCount);
        vkEnumeratePhysicalDevices(mInstance, &devCount, physDevs.data());

        // 最初のデバイスを使用する
        mPhysDev = physDevs[0];
        // メモリプロパティを取得しておく
        vkGetPhysicalDeviceMemoryProperties(mPhysDev, &mPhysMemProps);
    }

    uint32_t Device::searchGraphicsQueueIndex()
    {
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysDev, &propCount, nullptr);
        std::vector<VkQueueFamilyProperties> props(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysDev, &propCount, props.data());

        uint32_t graphicsQueue = ~0u;
        for (uint32_t i = 0; i < propCount; ++i)
          if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
          {
            graphicsQueue = i; 
            break;
          }

        return graphicsQueue;
    }

    void Device::createDevice()
    {
        const float defaultQueuePriority(1.0f);
        VkDeviceQueueCreateInfo devQueueCI{};
        devQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        devQueueCI.queueFamilyIndex = mGraphicsQueueIndex;
        devQueueCI.queueCount = 1;
        devQueueCI.pQueuePriorities = &defaultQueuePriority;

        std::vector<VkExtensionProperties> devExtProps;
        {
            // 拡張情報の取得.
            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, nullptr);
            devExtProps.resize(count);
            vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, devExtProps.data());
        }

        std::vector<const char *> extensions;
        for (const auto &v : devExtProps)
        {
            if (strcmp(v.extensionName, "VK_KHR_buffer_device_address"))
                extensions.push_back(v.extensionName);
        }

        std::cout << "\n\n\n";

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pQueueCreateInfos = &devQueueCI;
        ci.queueCreateInfoCount = 1;
        ci.ppEnabledExtensionNames = extensions.data();
        ci.enabledExtensionCount = uint32_t(extensions.size());

        auto result = vkCreateDevice(mPhysDev, &ci, nullptr, &mDevice);
        checkResult(result);

        // デバイスキューの取得
        vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mDeviceQueue);
    }

    void Device::prepareCommandPool()
    {
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = mGraphicsQueueIndex;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        auto result = vkCreateCommandPool(mDevice, &ci, nullptr, &mCommandPool);
        checkResult(result);
    }

    void Device::selectSurfaceFormat(VkFormat format)
    {
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, mSurface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, mSurface, &surfaceFormatCount, formats.data());

        // 検索して一致するフォーマットを見つける.
        for (const auto &f : formats)
        {
            if (f.format == format)
            {
                mSurfaceFormat = f;
            }
        }
    }

    void Device::createSwapchain(GLFWwindow* pWindow)
    {
        auto imageCount = (std::max)(2u, mSurfaceCaps.minImageCount);
        auto extent = mSurfaceCaps.currentExtent;
        if (extent.width == ~0u)
        {
            // 値が無効なのでウィンドウサイズを使用する.
            int width, height;
            glfwGetWindowSize(pWindow, &width, &height);
            extent.width = uint32_t(width);
            extent.height = uint32_t(height);
        }

        uint32_t queueFamilyIndices[] = {mGraphicsQueueIndex};
        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = mSurface;
        ci.minImageCount = imageCount;
        ci.imageFormat = mSurfaceFormat.format;
        ci.imageColorSpace = mSurfaceFormat.colorSpace;
        ci.imageExtent = extent;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.preTransform = mSurfaceCaps.currentTransform;
        ci.imageArrayLayers = 1;
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.queueFamilyIndexCount = 0;
        ci.presentMode = mPresentMode;
        ci.oldSwapchain = VK_NULL_HANDLE;
        ci.clipped = VK_TRUE;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        auto result = vkCreateSwapchainKHR(mDevice, &ci, nullptr, &mSwapchain);
        checkResult(result);
        mSwapchainExtent = extent;
    }

    void Device::createDepthBuffer()
    {
        VkImageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = VK_FORMAT_D32_SFLOAT;
        ci.extent.width = mSwapchainExtent.width;
        ci.extent.height = mSwapchainExtent.height;
        ci.extent.depth = 1;
        ci.mipLevels = 1;
        ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.arrayLayers = 1;
        auto result = vkCreateImage(mDevice, &ci, nullptr, &mDepthBuffer);
        checkResult(result);

        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(mDevice, mDepthBuffer, &reqs);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(mDevice, &ai, nullptr, &mDepthBufferMemory);
        vkBindImageMemory(mDevice, mDepthBuffer, mDepthBufferMemory, 0);
    }

    void Device::createViews()
    {
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
        mSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());
        mSwapchainViews.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = mSurfaceFormat.format;
            ci.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A,
            };
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            ci.image = mSwapchainImages[i];
            auto result = vkCreateImageView(mDevice, &ci, nullptr, &mSwapchainViews[i]);
            checkResult(result);
        }

        // for depthbuffer
        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = VK_FORMAT_D32_SFLOAT;
            ci.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A,
            };
            ci.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
            ci.image = mDepthBuffer;
            auto result = vkCreateImageView(mDevice, &ci, nullptr, &mDepthBufferView);
            checkResult(result);
        }
    }

    void Device::createRenderPass()
    {
        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        std::array<VkAttachmentDescription, 2> attachments;
        auto &colorTarget = attachments[0];
        auto &depthTarget = attachments[1];

        colorTarget = VkAttachmentDescription{};
        colorTarget.format = mSurfaceFormat.format;
        colorTarget.samples = VK_SAMPLE_COUNT_1_BIT;
        colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorTarget.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorTarget.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        depthTarget = VkAttachmentDescription{};
        depthTarget.format = VK_FORMAT_D32_SFLOAT;
        depthTarget.samples = VK_SAMPLE_COUNT_1_BIT;
        depthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthTarget.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthTarget.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthTarget.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthTarget.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference{}, depthReference{};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &colorReference;
        subpassDesc.pDepthStencilAttachment = &depthReference;

        ci.attachmentCount = uint32_t(attachments.size());
        ci.pAttachments = attachments.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &subpassDesc;

        auto result = vkCreateRenderPass(mDevice, &ci, nullptr, &mRenderPass);
        checkResult(result);
    }

    void Device::createFramebuffer()
    {
        VkFramebufferCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass = mRenderPass;
        ci.width = mSwapchainExtent.width;
        ci.height = mSwapchainExtent.height;
        ci.layers = 1;
        mFramebuffers.clear();
        for (auto &v : mSwapchainViews)
        {
            std::array<VkImageView, 2> attachments;
            ci.attachmentCount = uint32_t(attachments.size());
            ci.pAttachments = attachments.data();
            attachments[0] = v;
            attachments[1] = mDepthBufferView;

            VkFramebuffer framebuffer;
            auto result = vkCreateFramebuffer(mDevice, &ci, nullptr, &framebuffer);
            checkResult(result);
            mFramebuffers.push_back(framebuffer);
        }
    }

    void Device::prepareCommandBuffers()
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = mCommandPool;
        ai.commandBufferCount = uint32_t(mSwapchainViews.size());
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        mCommands.resize(ai.commandBufferCount);
        auto result = vkAllocateCommandBuffers(mDevice, &ai, mCommands.data());
        checkResult(result);

        // コマンドバッファのフェンスも同数用意する.
        mFences.resize(ai.commandBufferCount);
        VkFenceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (auto &v : mFences)
        {
            result = vkCreateFence(mDevice, &ci, nullptr, &v);
            checkResult(result);
        }
    }

    void Device::prepareSemaphores()
    {
        VkSemaphoreCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(mDevice, &ci, nullptr, &mRenderCompletedSem);
        vkCreateSemaphore(mDevice, &ci, nullptr, &mPresentCompletedSem);
    }

    uint32_t Device::getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
    {
        uint32_t result = ~0u;
        for (uint32_t i = 0; i < mPhysMemProps.memoryTypeCount; ++i)
        {
            if (requestBits & 1)
            {
                const auto &types = mPhysMemProps.memoryTypes[i];
                if ((types.propertyFlags & requestProps) == requestProps)
                {
                    result = i;
                    break;
                }
            }
            requestBits >>= 1;
        }
        return result;
    }

    void Device::enableDebugReport()
    {
        GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
        GetInstanceProcAddr(vkDebugReportMessageEXT);
        GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

        VkDebugReportFlagsEXT flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT drcCI{};
        drcCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drcCI.flags = flags;
        drcCI.pfnCallback = &DebugReportCallback;
        mvkCreateDebugReportCallbackEXT(mInstance, &drcCI, nullptr, &mDebugReport);
    }

    void Device::disableDebugReport()
    {
        if (mvkDestroyDebugReportCallbackEXT)
        {
            mvkDestroyDebugReportCallbackEXT(mInstance, mDebugReport, nullptr);
        }
    }

};     