#include <Device.hpp>

#include <iostream>
#include <vector>


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

    Result Device::initialize(const InitializeInfo& initializeInfo)
    {
        mInitializeInfo = initializeInfo;
        Result result;

        //GLFW初期化
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, 0);
        mWindow = glfwCreateWindow(
            mInitializeInfo.width,
            mInitializeInfo.height,
            mInitializeInfo.appName.c_str(),
            nullptr,
            nullptr
            );

        result = initializeInstance();
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = selectPhysicalDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = createDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = createCommandPool();
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = createSwapchain(mWindow);
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = createSemaphores();
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Device::checkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cout << "ERROR!\nvulkan error : " << result << "\n";
            return Result::eFailure;//ここをSwitchで分岐して独自結果を返す
        }

        return Result::eSuccess;
    }

    Result Device::initializeInstance()
    {
        Result result;

        std::vector<const char*> extensions;
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = mInitializeInfo.appName.c_str();
        appInfo.pEngineName = ENGINE_NAME;
        appInfo.apiVersion = VK_API_VERSION_1_1;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        // 拡張情報の取得
        std::vector<VkExtensionProperties> props;

        {
            uint32_t count = 0;
            result = checkResult(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }   

            props.resize(count);
            result = checkResult(vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data()));
            if (Result::eSuccess != result)
            {
                return result;
            }

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

        if(mInitializeInfo.debugFlag)
        {
            // デバッグ時には検証レイヤーを有効化
            const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
            ci.enabledLayerCount = 1;
            ci.ppEnabledLayerNames = layers;
        }

        // インスタンス生成
        result = checkResult(vkCreateInstance(&ci, nullptr, &mInstance));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return result;
    }

    Result Device::selectPhysicalDevice()
    {
        Result result;

        uint32_t devCount = 0;
        result = checkResult(vkEnumeratePhysicalDevices(mInstance, &devCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkPhysicalDevice> physDevs(devCount);
        result = checkResult(vkEnumeratePhysicalDevices(mInstance, &devCount, physDevs.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // 最初のデバイスを使用する
        mPhysDev = physDevs[0];
        // メモリプロパティを取得しておく
        vkGetPhysicalDeviceMemoryProperties(mPhysDev, &mPhysMemProps);

        return Result::eSuccess;
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

    Result Device::createDevice()
    {
        Result result;

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
            result = checkResult(vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }

            devExtProps.resize(count);
            result = checkResult(vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, devExtProps.data()));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        std::vector<const char *> extensions;
        for (const auto &v : devExtProps)
        {
            if (strcmp(v.extensionName, "VK_KHR_buffer_device_address"))
                extensions.push_back(v.extensionName);
        }

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.pQueueCreateInfos = &devQueueCI;
        ci.queueCreateInfoCount = 1;
        ci.ppEnabledExtensionNames = extensions.data();
        ci.enabledExtensionCount = uint32_t(extensions.size());

        result = checkResult(vkCreateDevice(mPhysDev, &ci, nullptr, &mDevice));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // デバイスキューの取得
        vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mDeviceQueue);

        return Result::eSuccess;
    }

    Result Device::createCommandPool()
    {
        Result result;

        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = mGraphicsQueueIndex;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        result = checkResult(vkCreateCommandPool(mDevice, &ci, nullptr, &mCommandPool));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Device::selectSurfaceFormat(VkFormat format)
    {
        Result result;

        uint32_t surfaceFormatCount = 0;//個数取得
        result = checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, mSurface, &surfaceFormatCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);//実際のフォーマット取得
        result = checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, mSurface, &surfaceFormatCount, formats.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // 検索して一致するフォーマット探す
        for (const auto &f : formats)
        {
            if (f.format == format)
            {
                mSurfaceFormat = f;
            }
        }

        return Result::eSuccess;
    }

    Result Device::createSwapchain(GLFWwindow* pWindow)
    {
        Result result;

        auto imageCount = (std::max)(2u, mSurfaceCaps.minImageCount);
        auto extent = mSurfaceCaps.currentExtent;
        if (extent.width == ~0u)
        {
            // 値が無効なのでウィンドウサイズを使用する
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

        result = checkResult(vkCreateSwapchainKHR(mDevice, &ci, nullptr, &mSwapchain));
        if (Result::eSuccess != result)
        {
            return result;
        }

        mSwapchainExtent = extent;

        return Result::eSuccess;
    }

    Result Device::createDepthBuffer()
    {
        Result result;

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
        //imageオブジェクト作成
        result = checkResult(vkCreateImage(mDevice, &ci, nullptr, &mDepthBuffer));
        if (Result::eSuccess != result)
        {
            return result;
        }


        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(mDevice, mDepthBuffer, &reqs);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(mDevice, &ai, nullptr, &mDepthBufferMemory);
        vkBindImageMemory(mDevice, mDepthBuffer, mDepthBufferMemory, 0);
    }

    Result Device::createViews()
    {
        Result result;

        uint32_t imageCount;
        result = checkResult(vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        mSwapchainImages.resize(imageCount);
        result = checkResult(vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

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
            if (Result::eSuccess != result)
            {
                return result;
            }
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
            result = checkResult(vkCreateImageView(mDevice, &ci, nullptr, &mDepthBufferView));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        return Result::eSuccess;
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

    Result Device::createSemaphores()
    {
        Result result;

        VkSemaphoreCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result = checkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mRenderCompletedSem));
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = checkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mPresentCompletedSem));
        if (Result::eSuccess != result)
        {
            return result;
        }
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

    Result Device::enableDebugReport()
    {
        std::cout << "Debug mode enabled.\n";

        GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
        GetInstanceProcAddr(vkDebugReportMessageEXT);
        GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

        VkDebugReportFlagsEXT flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT drcCI{};
        drcCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drcCI.flags = flags;
        drcCI.pfnCallback = &DebugReportCallback;
        mvkCreateDebugReportCallbackEXT(mInstance, &drcCI, nullptr, &mDebugReport);

        return Result::eSuccess;
    }

    void Device::disableDebugReport()
    {
        std::cout << "Debug mode disabled.\n";

        if (mvkDestroyDebugReportCallbackEXT)
        {
            mvkDestroyDebugReportCallbackEXT(mInstance, mDebugReport, nullptr);
        }
    }

};     