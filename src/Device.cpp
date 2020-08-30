#include <Device.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <variant>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define GetInstanceProcAddr(FuncName) \
    m##FuncName = reinterpret_cast<PFN_##FuncName>(vkGetInstanceProcAddr(mInstance, #FuncName))



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

    Device::~Device()
    {
        //destroy();
        return;
    }

    Result Device::initialize(const InitializeInfo& initializeInfo, std::vector<HSwapchain>& handlesRef)
    {
        mInitializeInfo = initializeInfo;
        Result result;

        std::cout << "initialize started...\n";

        //GLFW初期化
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, 0);

        std::cout << "GLFW initialized\n";

        //インスタンス作成
        result = createInstance();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "created VkInstance\n";

        if(mInitializeInfo.debugFlag)
        {
            result = enableDebugReport();
            if (Result::eSuccess != result)
            {
                return result;
            }
        }
        std::cout << "enabled debug report\n";

        //物理デバイス選択(いまのところ特に指定なし、先頭デバイスを使用する)
        result = selectPhysicalDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "selected Physical Device\n";

        //グラフィクスキューのインデックス検索(いまのところ特に条件指定はない)
        result = searchGraphicsQueueIndex();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "found index of GraphicsQueue\n";

        //論理デバイス作成
        result = createDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "created VkDevice\n";

        //コマンドプール作成
        result = createCommandPool();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "created VkCommandPool\n";

        //ウィンドウサーフェス、スワップチェイン作成
        {
            int time = 0;
            for (auto &w : mInitializeInfo.windows)
            {
                std::cout << "Swapchain No. " << time++ << "------\n";
                SwapchainObject so;

                so.mpWindow = std::make_optional(glfwCreateWindow(
                    w.width,
                    w.height,
                    (mInitializeInfo.appName + std::string(" ") + w.windowName).c_str(),
                    nullptr,
                    nullptr));

                result = createSurface(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cout << "created VkSurfaceKHR\n";

                result = createSwapchain(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cout << "created VkSwapChainKHR\n";

                result = createSwapchainImages(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cout << "created swapchain images\n";

                result = createDepthBuffer(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cout << "created swapchain depthbuffer\n";

                //スワップチェインオブジェクト格納
                mSwapchainMap.emplace(mNextSwapchainHandle, so);
                handlesRef.emplace_back(mNextSwapchainHandle++);
            }

        }

        result = createSemaphores();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cout << "created VkSemaphore\n";

        std::cout << "all initialize processes succeeded\n";
        return Result::eSuccess;
    }

    Result Device::destroy()
    {
        std::cout << "destroying...\n";

        if(VK_SUCCESS != vkDeviceWaitIdle(mDevice))
            std::cout << "device wait failed\n";

        // vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        // for (auto &v : mFramebuffers)
        // {
        //     vkDestroyFramebuffer(mDevice, v, nullptr);
        // }
        // mFramebuffers.clear();

        // vkFreeMemory(mDevice, mDepthBufferMemory, nullptr);
        // vkDestroyImage(mDevice, mDepthBuffer, nullptr);

        for(auto& e : mBufferMap)
        {
            if(e.second.mBuffer)
                vkDestroyBuffer(mDevice, e.second.mBuffer.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);
        }
        mBufferMap.clear();
        std::cout << "destroyed user allocated buffers\n";

        for(auto& e : mImageMap)
        {
            if (e.second.mImage)
                vkDestroyImage(mDevice, e.second.mImage.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);
            if (e.second.mView)
                vkDestroyImageView(mDevice, e.second.mView.value(), nullptr);
            //if(e.second.mFrameBuffer)
            //    vkDestroyFramebuffer(mDevice, e.second.mFrameBuffer.value(), nullptr);

			if(e.second.mSampler)
				vkDestroySampler(mDevice, e.second.mSampler.value(), nullptr);
        }
        mImageMap.clear();
        std::cout << "destroyed user allocated textures\n";
        std::cout << "destroyed user allocated sampler\n";

        for(auto& e : mRDSTMap)
        {
            for(auto& f : e.second.mFramebuffers)
                vkDestroyFramebuffer(mDevice, f.value(), nullptr);

            vkDestroyRenderPass(mDevice, e.second.mRenderPass.value(), nullptr);
        }

        for(auto& e : mRPMap)
        {
            vkDestroyDescriptorSetLayout(mDevice, e.second.mDescriptorSetLayout.value(), nullptr);
            vkDestroyPipelineLayout(mDevice, e.second.mPipelineLayout.value(), nullptr);
            vkDestroyPipeline(mDevice, e.second.mPipeline.value(), nullptr);
        }

        //for(auto& e : mSamplerMap)
        //{
        //    vkDestroySampler(mDevice, e.second, nullptr);
        //}
        //mSamplerMap.clear();
        

        for (auto &v : mFences)
        {
            vkDestroyFence(mDevice, v, nullptr);
        }
        mFences.clear();
        std::cout << "destroyed fences\n";

        
        vkDestroySemaphore(mDevice, mPresentCompletedSem, nullptr);
        vkDestroySemaphore(mDevice, mRenderCompletedSem, nullptr);
        std::cout << "destroyed semaphores\n";

        if(mCommands.size() > 0)//現状こうしてある
        {
            vkFreeCommandBuffers(mDevice, mCommandPool, uint32_t(mCommands.size()), mCommands.data());
            mCommands.clear();
            std::cout << "destroyed command buffers\n";
        }

        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        std::cout << "destroyed command pool\n";

        for(auto& so : mSwapchainMap)
        {
            //std::cout << "destroyed swapchain imageViews\n";
            
            vkDestroySwapchainKHR(mDevice, so.second.mSwapchain.value(), nullptr);
            std::cout << "destroyed swapchain\n";

            vkDestroySurfaceKHR(mInstance, so.second.mSurface.value(), nullptr);
            std::cout << "destroyed surface\n";
        }
        std::cout << "destroyed all swapchain \n";

        vkDestroyDevice(mDevice, nullptr);
        std::cout << "destroyed device\n";

        if(mInitializeInfo.debugFlag)
            disableDebugReport();

        vkDestroyInstance(mInstance, nullptr);
        std::cout << "destroyed instance\n";

        std::cout << "all destroying process succeeded\n";

        return Result::eSuccess;
    }

    Result Device::checkVkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cout << "ERROR!\nvulkan error : " << result << "\n";
            return Result::eFailure;//ここをSwitchで分岐して独自結果を返す?
        }

        return Result::eSuccess;
    }

    Result Device::createInstance()
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
            result = checkVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }   

            props.resize(count);
            result = checkVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data()));
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
        else
        {
            ci.enabledLayerCount = 0;
            ci.ppEnabledLayerNames = nullptr;
        }

        // インスタンス生成
        result = checkVkResult(vkCreateInstance(&ci, nullptr, &mInstance));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Device::selectPhysicalDevice()
    {
        Result result;

        uint32_t devCount = 0;
        result = checkVkResult(vkEnumeratePhysicalDevices(mInstance, &devCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkPhysicalDevice> physDevs(devCount);
        result = checkVkResult(vkEnumeratePhysicalDevices(mInstance, &devCount, physDevs.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::cout << "Physical Device number : " << physDevs.size() << "\n";

        // 最初のデバイスを使用する
        mPhysDev = physDevs[0];
        // メモリプロパティを取得しておく
        vkGetPhysicalDeviceMemoryProperties(mPhysDev, &mPhysMemProps);

        return Result::eSuccess;
    }

    Result Device::searchGraphicsQueueIndex()
    {
        uint32_t propCount;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysDev, &propCount, nullptr);
        std::vector<VkQueueFamilyProperties> props(propCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysDev, &propCount, props.data());

        for (uint32_t i = 0; i < propCount; ++i)
            if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                mGraphicsQueueIndex = i;
                break;
            }

        return Result::eSuccess;
    }

    Result Device::createDevice()
    {
        Result result;

        std::vector<VkExtensionProperties> devExtProps;

        {
            // 拡張情報の取得.
            uint32_t count = 0;
            result = checkVkResult(vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }

            devExtProps.resize(count);
            result = checkVkResult(vkEnumerateDeviceExtensionProperties(mPhysDev, nullptr, &count, devExtProps.data()));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }
        
        {
            const float defaultQueuePriority(1.0f);
            VkDeviceQueueCreateInfo devQueueCI{};
            devQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            devQueueCI.queueFamilyIndex = mGraphicsQueueIndex;
            devQueueCI.queueCount = 1;
            devQueueCI.pQueuePriorities = &defaultQueuePriority;

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

            result = checkVkResult(vkCreateDevice(mPhysDev, &ci, nullptr, &mDevice));
            if (Result::eSuccess != result)
            {
                return result;
            }
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

        result = checkVkResult(vkCreateCommandPool(mDevice, &ci, nullptr, &mCommandPool));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Device::createSurface(SwapchainObject* pSO)
    {
        Result result;

        VkSurfaceKHR surface;
        result = checkVkResult(glfwCreateWindowSurface(mInstance, pSO->mpWindow.value(), nullptr, &surface));
        if(Result::eSuccess != result)
        {
            return result;
        }
        pSO->mSurface = surface;

        // サーフェスのフォーマット情報選択
        result = selectSurfaceFormat(pSO, VK_FORMAT_B8G8R8A8_UNORM);
        if (Result::eSuccess != result)
        {
            return result;
        }

        // サーフェスの能力値情報取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysDev, pSO->mSurface.value(), &pSO->mSurfaceCaps));
        if (Result::eSuccess != result)
        {
            return result;
        }

        VkBool32 isSupport;
        result = checkVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysDev, mGraphicsQueueIndex, pSO->mSurface.value(), &isSupport));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Device::selectSurfaceFormat(SwapchainObject* pSO, VkFormat format)
    {
        Result result;

        uint32_t surfaceFormatCount = 0;//個数取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, pSO->mSurface.value(), &surfaceFormatCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);//実際のフォーマット取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, pSO->mSurface.value(), &surfaceFormatCount, formats.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        //一致するフォーマットを検索
        for (const auto &f : formats)
        {
            if (f.format == format)
            {
                pSO->mSurfaceFormat = f;
            }
        }

        return Result::eSuccess;
    }

    Result Device::createSwapchain(SwapchainObject* pSO)
    {
        Result result;

        uint32_t imageCount = (std::max)(pSO->mSurfaceCaps.minImageCount, mInitializeInfo.frameCount);
        auto extent = pSO->mSurfaceCaps.currentExtent;
        if (extent.width == ~0u)
        {
            // 値が無効なのでウィンドウサイズを使用する
            int width, height;
            glfwGetWindowSize(pSO->mpWindow.value(), &width, &height);
            extent.width = uint32_t(width);
            extent.height = uint32_t(height);
        }

        pSO->mPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t queueFamilyIndices[] = {mGraphicsQueueIndex};
        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = pSO->mSurface.value();
        ci.minImageCount = imageCount;
        ci.imageFormat = pSO->mSurfaceFormat.format;
        ci.imageColorSpace = pSO->mSurfaceFormat.colorSpace;
        ci.imageExtent = extent;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.preTransform = pSO->mSurfaceCaps.currentTransform;
        ci.imageArrayLayers = 1;
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.queueFamilyIndexCount = 0;
        ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        ci.oldSwapchain = VK_NULL_HANDLE;
        ci.clipped = VK_TRUE;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        {
            VkSwapchainKHR swapchain;
            result = checkVkResult(vkCreateSwapchainKHR(mDevice, &ci, nullptr, &swapchain));
            if (Result::eSuccess != result)
            {
                return result;
            }

            pSO->mSwapchain = swapchain;
        }

        pSO->mSwapchainExtent = extent;

        return Result::eSuccess;
    }

    Result Device::createSwapchainImages(SwapchainObject* pSO)
    {
        Result result;

        uint32_t imageCount;
        result = checkVkResult(vkGetSwapchainImagesKHR(mDevice, pSO->mSwapchain.value(), &imageCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkImage> swapchainImages(imageCount);
        result = checkVkResult(vkGetSwapchainImagesKHR(mDevice, pSO->mSwapchain.value(), &imageCount, swapchainImages.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkImageView> swapchainViews(imageCount);

        for (size_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = pSO->mSurfaceFormat.format;
            ci.components =
                {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A,
                };
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            ci.image = swapchainImages[i];
            result = checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &swapchainViews[i]));
            if (Result::eSuccess != result)
            {
                std::cout << "failed to create swapchain image views!\n";
                return result;
            }
        }

        for (size_t i = 0; i < imageCount; ++i)
        {
            ImageObject io;
            io.mImage = swapchainImages[i];
            io.mView = swapchainViews[i];
            io.usage = TextureUsage::eSwapchainImage;
            io.mIsHostVisible = false;
            io.extent = { pSO->mSwapchainExtent.width, pSO->mSwapchainExtent.height, static_cast<uint32_t>(1)};
            io.format = pSO->mSurfaceFormat.format;

            pSO->mHSwapchainImages.emplace_back(mNextTextureHandle++);
            mImageMap.emplace(pSO->mHSwapchainImages.back(), io);
        }

        return Result::eSuccess;
    }

    Result Device::createSemaphores()
    {
        Result result;

        VkSemaphoreCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mRenderCompletedSem));
        if (Result::eSuccess != result)
        {
            return result;
        }

        result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mPresentCompletedSem));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    // Result Device::getSwapchainImages(HSwapchain handle, std::vector<HTexture>& handlesRef)
    // {
    //     if(mSwapchainMap.count(handle) <= 0)
    //     {
    //         return Result::eFailure;
    //     }

    //     auto &images = mSwapchainMap[handle].mHSwapchainImages;;
    //     handlesRef.resize(images.size());
    //     for(size_t i = 0; i < handlesRef.size(); ++i)
    //     {
    //         handlesRef.at(i) = images[i];
    //     }

    //     return Result::eSuccess;
    // }

    // Result Device::getSwapchainDepthBuffer(HSwapchain handle, HTexture *pHandle)
    // {
    //     if(mSwapchainMap.count(handle) <= 0)
    //     {
    //         return Result::eFailure;
    //     }

    //     *pHandle = mSwapchainMap[handle].mHDepthBuffer;

    //     return Result::eSuccess;
    // }

    Result Device::enableDebugReport()
    {
        std::cout << "enabled debug mode\n";

        GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
        GetInstanceProcAddr(vkDebugReportMessageEXT);
        GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

        VkDebugReportFlagsEXT flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT drcci{};
        drcci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drcci.flags = flags;
        drcci.pfnCallback = &DebugReportCallback;
        mvkCreateDebugReportCallbackEXT(mInstance, &drcci, nullptr, &mDebugReport);

        return Result::eSuccess;
    }

    Result Device::disableDebugReport()
    {

        if (mvkDestroyDebugReportCallbackEXT)
        {
            mvkDestroyDebugReportCallbackEXT(mInstance, mDebugReport, nullptr);
        }

        std::cout << " disabled debug mode\n";

        return Result::eSuccess;
    }

    uint32_t Device::getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
    {
        uint32_t result = 0;
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

    Result Device::createBuffer(const BufferInfo &info, HBuffer *pHandle)
    {
        Result result = Result::eFailure;
        BufferObject bo;

        {
            VkBufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

            switch (info.usage)
            {
            case BufferUsage::eVertex:
                ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                break;
            case BufferUsage::eIndex:
                ci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                break;
            case BufferUsage::eUniform:
                ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                break;
            default:
                std::cout << "buffer usage is not descibed.\n";
                return Result::eFailure;
                break;
            }

            if (info.size <= 0)
            {
                return Result::eFailure;
            }

            ci.size = info.size;
            ci.pNext = nullptr;

            {
                VkBuffer buffer;
                result = checkVkResult(vkCreateBuffer(mDevice, &ci, nullptr, &buffer));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                bo.mBuffer = buffer;
            }
        }

        {
            VkMemoryPropertyFlagBits fb;
            if (info.isHostVisible)
            {
                fb = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                bo.mIsHostVisible = true;
            }
            else
            {
                fb = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                bo.mIsHostVisible = false;
            }

            VkMemoryRequirements reqs;
            vkGetBufferMemoryRequirements(mDevice, bo.mBuffer.value(), &reqs);
            VkMemoryAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = reqs.size;
            // 基本的にメモリタイプは指定
            ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, fb);
            // メモリの確保
            {
                VkDeviceMemory memory;
                result = checkVkResult(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                bo.mMemory = memory;
            }

            // メモリのバインド
            result = checkVkResult(vkBindBufferMemory(mDevice, bo.mBuffer.value(), bo.mMemory.value(), 0));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        *pHandle = mNextBufferHandle++;

        mBufferMap.emplace(*pHandle, bo);

        return Result::eSuccess;
    }

    Result Device::writeBuffer(const size_t size, const void *const pData, const HBuffer &handle)
    {
        Result result = Result::eFailure;
        if(mBufferMap.count(handle) <= 0)
            return result;

        BufferObject &bo = mBufferMap[handle];

        void *p;//マップ先アドレス
        
        result = checkVkResult(vkMapMemory(mDevice, bo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
        if(result != Result::eSuccess)
            return result;
        memcpy(p, pData, size);
        vkUnmapMemory(mDevice, bo.mMemory.value());

        return Result::eSuccess;
    }

    Result Device::createTexture(const TextureInfo &info, HTexture *pHandle)
    {
		Result result;
		ImageObject io;

		{
			VkImageCreateInfo ci{};

			ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			ci.pNext = nullptr;
			switch (info.format.first)
			{
			case ResourceType::eFVec3:
				switch (info.format.second)
				{
				case TextureFormatType::eUNorm:
					io.format = VK_FORMAT_R8G8B8_UNORM;
					break;
				case TextureFormatType::eFloat:
					io.format = VK_FORMAT_R16G16B16_SFLOAT;
					break;
				default:
					return Result::eFailure;
				}
				break;
			case ResourceType::eFVec4:
				switch (info.format.second)
				{
				case TextureFormatType::eUNorm:
					io.format = VK_FORMAT_R8G8B8A8_UNORM;
					break;
				case TextureFormatType::eFloat:
					io.format = VK_FORMAT_R16G16B16A16_SFLOAT;
					break;
				default:
					return Result::eFailure;
					break;
				}
				break;
			default:
				return Result::eFailure;
				break;
			}

			ci.format = io.format;

			switch (info.dimention)
			{
			case TextureDimention::e2D:
				ci.imageType = VK_IMAGE_TYPE_2D;
				ci.extent = { uint32_t(info.width), uint32_t(info.height), 1 };
				if (info.depth != 1)
					std::cout << "ignored invalid depth param\n";
				break;
				// case TextureDimention::e3D:
				//     ci.imageType = VK_IMAGE_TYPE_3D;
				//     ci.extent = {uint32_t(info.width), uint32_t(info.height), uint32_t(info.depth)};
				//     break;
			default:
				return Result::eFailure;
				break;
			}

			switch (info.usage)
			{
			case TextureUsage::eShaderResource:
				ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				break;
			case TextureUsage::eColorTarget:
				ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				break;
			case TextureUsage::eDepthStencilTarget:
				ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				break;
				// case TextureUsage::eUnordered: //不定なのでこまった
				//     ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				//     break;
			default:
				return Result::eFailure;
				break;
			}

			io.usage = info.usage;

			ci.arrayLayers = 1;
			ci.mipLevels = 1;
			ci.samples = VK_SAMPLE_COUNT_1_BIT;
			ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;

            io.extent = ci.extent;

            {
                VkImage image;
                result = checkVkResult(vkCreateImage(mDevice, &ci, nullptr, &image));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                io.mImage = image;
            }
        }

		// メモリ量の算出
		VkMemoryRequirements reqs;
		vkGetImageMemoryRequirements(mDevice, io.mImage.value(), &reqs);
		VkMemoryAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ai.allocationSize = reqs.size;
		// メモリタイプの判定
		VkMemoryPropertyFlagBits fb;
		if (info.isHostVisible)
		{
			fb = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			io.mIsHostVisible = false;
		}
		else
		{
			fb = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			io.mIsHostVisible = true;
		}

		ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, fb);
		// メモリの確保
        {
            VkDeviceMemory memory;
            vkAllocateMemory(mDevice, &ai, nullptr, &memory);

            io.mMemory = memory;
        }
        // メモリのバインド
		vkBindImageMemory(mDevice, io.mImage.value(), io.mMemory.value(), 0);

		{
			// テクスチャ参照用のビューを生成
			VkImageViewCreateInfo ci{};
			ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

			switch (info.dimention)
			{
			case TextureDimention::e2D:
				ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
				// case TextureDimention::e3D:
				//     ci.viewType = VK_IMAGE_VIEW_TYPE_3D;
				//     break;
			default:
				return Result::eFailure;
				break;
			}

			//ビュー作成がやばい
			ci.image = io.mImage.value();
			ci.format = io.format;
			ci.components =
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A,
			};

			switch (info.usage)
			{
			case TextureUsage::eDepthStencilTarget:
				ci.subresourceRange = {
					VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
				break;
			default:
				ci.subresourceRange = {
					VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				break;
			}

            {
                VkImageView imageView;
                if (VK_SUCCESS != vkCreateImageView(mDevice, &ci, nullptr, &imageView))
                {
                    std::cout << "failed to create vkImageView!\n";
                    exit(-1);
                }

                io.mView = imageView;
            }
        }

        {
            VkSamplerCreateInfo sci{};//適当なサンプラー
            sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.pNext = nullptr;
            sci.minFilter = VK_FILTER_LINEAR;
            sci.magFilter = VK_FILTER_LINEAR;
            sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.maxAnisotropy = 1.f;
            sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VkSampler sampler;
            result = checkVkResult(vkCreateSampler(mDevice, &sci, nullptr, &sampler));

            io.mSampler = sampler;
        }

        *pHandle = mNextTextureHandle++;
        mImageMap.emplace(*pHandle, io);

        return Result::eSuccess;
    }

    Result Device::createTextureFromFile(const char* fileName, HTexture* pHandle)
    {
        ImageObject io;
        Result result;

        stbi_uc *pImage = nullptr;
        int width = 0, height = 0, channel = 0;
        io.format = VK_FORMAT_R8G8B8A8_UNORM;//固定する
        io.usage = TextureUsage::eShaderResource;

        {
            VkImageCreateInfo ci{};

            if (!fileName)
            {
                std::cout << "invalid file name\n";
                return Result::eFailure;
            }
    
            pImage = stbi_load(fileName, &width, &height, nullptr, 4);
            channel = 4;
            if (!pImage)
            {
                std::cout << "Failed to load texture file!\n";
                return Result::eFailure;
            }

            io.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(1)};

            // テクスチャのVkImageを生成
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.extent = {uint32_t(width), uint32_t(height), 1};
            ci.format = VK_FORMAT_R8G8B8A8_UNORM;
            ci.imageType = VK_IMAGE_TYPE_2D;
            ci.arrayLayers = 1;
            ci.mipLevels = 1;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            {
                VkImage image;
                result = checkVkResult(vkCreateImage(mDevice, &ci, nullptr, &image));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                io.mImage = image;
            }
        }

        uint32_t imageSize = width * height * channel;

        {
            VkSamplerCreateInfo sci{}; //適当なサンプラー
            sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.pNext = nullptr;
            sci.minFilter = VK_FILTER_LINEAR;
            sci.magFilter = VK_FILTER_LINEAR;
            sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.maxAnisotropy = 1.f;
            sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VkSampler sampler;
            result = checkVkResult(vkCreateSampler(mDevice, &sci, nullptr, &sampler));
            io.mSampler = sampler;
        }

        *pHandle = mNextTextureHandle++;
        mImageMap.emplace(*pHandle, io);

        writeTexture(pImage, *pHandle);

        if (pImage != nullptr)
                stbi_image_free(pImage);

        return Result::eSuccess;
    }

    Result Device::writeTexture(const void *const pData, const HTexture &handle)
    {
        Result result = Result::eFailure;

        if(mImageMap.count(handle) <= 0)
            return Result::eFailure;

        ImageObject& io = mImageMap[handle];

        // ステージングバッファを用意
        BufferObject stagingBo;

        {
            size_t imageSize = io.extent.width * io.extent.height * io.extent.depth;
            VkBufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ci.size = imageSize;
            ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            {
                VkBuffer buffer;
                vkCreateBuffer(mDevice, &ci, nullptr, &buffer);
                stagingBo.mBuffer = buffer;
            }

            {
                VkMemoryRequirements reqs;
                vkGetBufferMemoryRequirements(mDevice, stagingBo.mBuffer.value(), &reqs);
                VkMemoryAllocateInfo ai{};
                ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ai.allocationSize = reqs.size;
                ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                //メモリ確保
                {
                    VkDeviceMemory memory;
                    vkAllocateMemory(mDevice, &ai, nullptr, &memory);
                    stagingBo.mMemory = memory;
                }

                // メモリバインド
                vkBindBufferMemory(mDevice, stagingBo.mBuffer.value(), stagingBo.mMemory.value(), 0);
            }

            void *p;
            result = checkVkResult(vkMapMemory(mDevice, stagingBo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
            if (Result::eSuccess != result)
                return result;
            
            memcpy(p, &io.mMemory.value(), imageSize);
            vkUnmapMemory(mDevice, stagingBo.mMemory.value());
        }

        VkBufferImageCopy copyRegion{};
        copyRegion.imageExtent = 
        {   
            static_cast<uint32_t>(io.extent.width), 
            static_cast<uint32_t>(io.extent.height), 
            static_cast<uint32_t>(io.extent.depth)
        };
        copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        VkCommandBuffer command;
        {
            VkCommandBufferAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandBufferCount = 1;
            ai.commandPool = mCommandPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            vkAllocateCommandBuffers(mDevice, &ai, &command);
        }

        VkCommandBufferBeginInfo commandBI{};
        commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command, &commandBI);
        setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(command, stagingBo.mBuffer.value(), io.mImage.value(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkEndCommandBuffer(command);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command;
        vkQueueSubmit(mDeviceQueue, 1, &submitInfo, VK_NULL_HANDLE);

        //コピー終了
        vkDeviceWaitIdle(mDevice);
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &command);

        // ステージングバッファ解放
        vkFreeMemory(mDevice, stagingBo.mMemory.value(), nullptr);
        vkDestroyBuffer(mDevice, stagingBo.mBuffer.value(), nullptr);

        return Result::eSuccess;
    }

    Result Device::setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier imb{};
        imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.oldLayout = oldLayout;
        imb.newLayout = newLayout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imb.image = image;

        // パイプライン中でリソースへの書込み最終のステージ
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        // パイプライン中で次にリソースに書き込むステージ
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        switch (oldLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            imb.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        }

        switch (newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        }

        vkCmdPipelineBarrier(
            command,
            srcStage,
            dstStage,
            0,
            0, // memoryBarrierCount
            nullptr,
            0, // bufferMemoryBarrierCount
            nullptr,
            1, // imageMemoryBarrierCount
            &imb);

        return Result::eSuccess;
    }

    Result Device::createCommandBuffers()
    {
        Result result;

        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = mCommandPool;
        ai.commandBufferCount = uint32_t(mInitializeInfo.frameCount);
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        mCommands.resize(ai.commandBufferCount);
        result = checkVkResult(vkAllocateCommandBuffers(mDevice, &ai, mCommands.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // コマンドバッファのフェンスも同数用意する.
        mFences.resize(ai.commandBufferCount);
        VkFenceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (auto &v : mFences)
        {
            result = checkVkResult(vkCreateFence(mDevice, &ci, nullptr, &v));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        return Result::eSuccess;
    }

    
    Result Device::createDepthBuffer(SwapchainObject *pSO)
    {
        Result result;
        ImageObject io;

        {
            VkImageCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.imageType = VK_IMAGE_TYPE_2D;
            ci.format = VK_FORMAT_D32_SFLOAT;
            ci.extent.width = pSO->mSwapchainExtent.width;
            ci.extent.height = pSO->mSwapchainExtent.height;
            ci.extent.depth = 1;
            ci.mipLevels = 1;
            ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.arrayLayers = 1;

            //ImageObjectの保有データ設定
            io.format = ci.format;
            io.extent = ci.extent;
            //imageオブジェクト作成
            {
                VkImage image;
                result = checkVkResult(vkCreateImage(mDevice, &ci, nullptr, &image));
                if (Result::eSuccess != result)
                {
                    std::cout << "failed to create depth buffer's image\n";
                    return result;
                }
                io.mImage = image;
            }
        }

        {
            VkMemoryRequirements reqs;
            vkGetImageMemoryRequirements(mDevice, io.mImage.value(), &reqs);
            VkMemoryAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = reqs.size;
            ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            
            {
                VkDeviceMemory memory;
                result = checkVkResult(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
                if (Result::eSuccess != result)
                {
                    return result;
                }
                io.mMemory = memory;
            }

            result = checkVkResult(vkBindImageMemory(mDevice, io.mImage.value(), io.mMemory.value(), 0));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = io.mImage.value();
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = io.format;
            ci.components =
            {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A,
            };
            ci.subresourceRange = 
            {
                VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1
            };

            VkImageView imageView;
            result = checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &imageView));
            if (Result::eSuccess != result)
            {
                std::cout << "failed to create vkImageView!\n";
                return result;
            }

            io.mView = imageView;
        }

        io.usage = TextureUsage::eDepthStencilTarget;
        mImageMap.emplace(mNextTextureHandle, io);
        pSO->mHDepthBuffer = mNextTextureHandle++;

        return Result::eSuccess;
    }

    Result Device::createShaderModule(Shader shader, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo *pSSCI)
    {
        Result result;

        std::ifstream infile(shader.path.c_str(), std::ios::binary);
        if (!infile)
        {
            std::cout << "failed to load file from " << shader.path <<  " \n";
            exit(-1);
        }
        
        std::vector<char> filedata;
        filedata.resize(uint32_t(infile.seekg(0, std::ifstream::end).tellg()));
        infile.seekg(0, std::ifstream::beg).read(filedata.data(), filedata.size());

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.pCode = reinterpret_cast<uint32_t *>(filedata.data());
        ci.codeSize = filedata.size();
        result = checkVkResult(vkCreateShaderModule(mDevice, &ci, nullptr, &shaderModule));
        if(result != Result::eSuccess)
        {
            std::cout << "failed to create Shader Module\n";
            return result;
        }

        pSSCI->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pSSCI->flags = 0;
        pSSCI->pNext = nullptr;
        pSSCI->stage = stage;
        pSSCI->module = shaderModule;
        pSSCI->pName = shader.entryPoint.c_str();

        return Result::eSuccess;
    }


    Result Device::createRenderDSTFromSwapchain(const HSwapchain &handle, bool depthTestEnable, HRenderDST *pHandle)
    {
        Result result;
        RenderDSTObject rdsto;

        //Renderpass, 対象に応じたFramebuffer構築

        if(mSwapchainMap.count(handle) <= 0)
        {
            std::cout << "invalid swapchain handle\n";
            return Result::eFailure;
        }

        rdsto.mHSwapchain = handle;
        auto& swapchain = mSwapchainMap[handle];

        {
            VkExtent3D extent;
            extent.width = swapchain.mSwapchainExtent.width;
            extent.height = swapchain.mSwapchainExtent.height;
            extent.depth = 1.0f;
            rdsto.mExtent = extent;
        }

        {
            VkRenderPassCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            std::vector<VkAttachmentDescription> adVec;
            VkAttachmentReference ar;
            VkAttachmentReference depthAr;

            //adVec.reserve(2);

            auto rdst = swapchain.mHSwapchainImages[0]; //情報得るだけなのでどれでもいい

            if (mImageMap.count(rdst) <= 0)
            {
                std::cout << "invalid swapchain image handle\n";
                return Result::eFailure;
            }

            auto& io = mImageMap[rdst];
            adVec.emplace_back();
            adVec.back().format = io.format;
            adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
            adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            adVec.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            ar.attachment = 0;
            ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassDesc{};
            subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.pColorAttachments = &ar;


            //指定されていれば末尾にDSBを追加
            if (depthTestEnable)
            {
                adVec.emplace_back();

                auto &depthBuffer = mImageMap[swapchain.mHDepthBuffer];
                adVec.back().format = depthBuffer.format;
                adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
                adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                adVec.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAr.attachment = 1;
                depthAr.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                subpassDesc.pDepthStencilAttachment = &depthAr;
            }
            else
            {
                subpassDesc.pDepthStencilAttachment = nullptr;
            }

            ci.attachmentCount = adVec.size();
            ci.pAttachments = adVec.data();
            ci.subpassCount = 1;
            ci.pSubpasses = &subpassDesc;
            {
                VkRenderPass renderPass;
                result = checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
                if (Result::eSuccess != result)
                {
                    return result;
                }
                rdsto.mRenderPass = std::make_optional<VkRenderPass>(renderPass);
            }
        }


        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = rdsto.mRenderPass.value();
        fbci.width = swapchain.mSwapchainExtent.width;
        fbci.height = swapchain.mSwapchainExtent.height;
        fbci.layers = 1;
 
        if (depthTestEnable)
        {
            const auto &depth = mImageMap[swapchain.mHDepthBuffer];
            for (auto &h : swapchain.mHSwapchainImages)
            {
                
                const auto &img = mImageMap[h];
                std::array<VkImageView, 2> ivArr = {img.mView.value(), depth.mView.value()};
                fbci.attachmentCount = 2;
                fbci.pAttachments = ivArr.data();

                rdsto.mFramebuffers.emplace_back();
                {
                    VkFramebuffer framebuffer;
                    result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &framebuffer));
                    if (Result::eSuccess != result)
                    {
                        return result;
                    }
                    rdsto.mFramebuffers.back() = framebuffer;
                }
            }
        }
        else
        {
            fbci.attachmentCount = 1;
            for (auto &h : swapchain.mHSwapchainImages)
            {
                const auto &img = mImageMap[h];
                VkImageView iv = img.mView.value();
                fbci.pAttachments = &iv;

                rdsto.mFramebuffers.emplace_back();
                VkFramebuffer frameBuffer = rdsto.mFramebuffers.back().value();
                result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
                if (Result::eSuccess != result)
                {
                    return result;
                }
            }
        }

        *pHandle = mNextRenderDSTHandle++;
        mRDSTMap.emplace(*pHandle, rdsto);

        return Result::eSuccess;
    }

    Result Device::createRenderDSTFromTextures(const std::vector<HTexture> textures, HRenderDST *pHandle)
    {
		Result result;
		RenderDSTObject rdsto;

        rdsto.mHSwapchain = std::nullopt;

        VkRenderPassCreateInfo ci{};
        std::vector<VkAttachmentDescription> adVec;
        std::vector<VkAttachmentReference> arVec;
        std::optional<HTexture> hDepthBuffer;

        if(mInitializeInfo.debugFlag)//チェック
        {
            for (auto &tex : textures)
            {
                if (mImageMap.count(tex) <= 0)
                {
                    std::cout << "invalid texture handle\n";
                    return Result::eFailure;
                }
                auto &io = mImageMap[tex];

                //使いみちが違うのはアウト
                if (io.usage != TextureUsage::eColorTarget || io.usage != TextureUsage::eDepthStencilTarget)
                {
                    std::cout << "invalid texture usage\n";
                    return Result::eFailure;
                }

                //サイズが違うのもアウト
                if (!rdsto.mExtent)
                    rdsto.mExtent = io.extent;
                if (rdsto.mExtent.value().width != io.extent.width
                &&  rdsto.mExtent.value().height != io.extent.height
                &&  rdsto.mExtent.value().depth != io.extent.depth)
                {
                    std::cout << "invalid texture extent\n";
                    return Result::eFailure;
                }
            }
        }

        //Renderpass, 対象に応じたFramebuffer構築
        for(auto& tex : textures)
        {
            
            auto &io = mImageMap[tex];
            adVec.emplace_back();
            arVec.emplace_back();

            if(!rdsto.mExtent)
                rdsto.mExtent = io.extent;
            

            adVec.back().format = io.format;
            adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
            adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            arVec.back().attachment = 0;

            switch (io.usage)
            {
            case TextureUsage::eColorTarget:
                adVec.back().finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                arVec.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;
            case TextureUsage::eDepthStencilTarget:
                hDepthBuffer = tex;//デプスバッファ登録して情報は消しとく
                adVec.pop_back();
                arVec.pop_back();
                break;
            default: //くるわけないのであるが...
                return Result::eFailure;
                break;
            }
        }
            
        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount = arVec.size();
        subpassDesc.pColorAttachments = arVec.data();

        //指定されていれば末尾にDSBを追加
        if (hDepthBuffer)
        {
            VkAttachmentReference depthAr;
            adVec.emplace_back();
            auto &depthBuffer = mImageMap[hDepthBuffer.value()];

            adVec.back().format = depthBuffer.format;
            adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
            adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            adVec.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAr.attachment = 1;
            depthAr.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpassDesc.pDepthStencilAttachment = &depthAr;
        }

        ci.attachmentCount = adVec.size();
        ci.pAttachments = adVec.data();
        ci.subpassCount = 1;
        ci.pSubpasses = &subpassDesc;

        {
            auto renderPass = rdsto.mRenderPass.value();
            result = checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = rdsto.mRenderPass.value();
        fbci.width = rdsto.mExtent.value().width;
        fbci.height = rdsto.mExtent.value().height;
        fbci.layers = 1;

        fbci.attachmentCount = rdsto.mTargetNum = adVec.size();
        std::vector<VkImageView> ivVec;
        for (auto &tex : textures)
        {
            const auto &img = mImageMap[tex];
            ivVec.emplace_back(img.mView.value());
        }

        fbci.pAttachments = ivVec.data();
        rdsto.mFramebuffers.emplace_back();
        result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &rdsto.mFramebuffers.back().value()));
        if(Result::eSuccess != result)
        {
            return result;
        }

        *pHandle = mNextRenderDSTHandle++;
        mRDSTMap.emplace(*pHandle, rdsto);

        return Result::eSuccess;
    }

    Result Device::createRenderPipeline(const RenderPipelineInfo &info, HRenderPipeline *pHandle)
    {
        Result result;
        RenderPipelineObject rpo;

        if(mRDSTMap.count(info.renderDST) <= 0)
        {
            std::cout << "invalid renderDST handle\n";
            return Result::eFailure;
        }

        RenderDSTObject rdsto = mRDSTMap[info.renderDST];
        rpo.mHRenderDST = info.renderDST;

        //PSO構築
        {
            //頂点バインディング
            VkVertexInputBindingDescription ib;
            {
                ib.binding = 0;
                ib.stride = info.vertexLayout.sizeOfType;
                ib.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            }

            //頂点属性
            std::vector<VkVertexInputAttributeDescription> ia_vec;
            {
                if (info.vertexLayout.layouts.size() <= 0)
                {
                    return Result::eFailure;
                }

                for (size_t i = 0; i < info.vertexLayout.layouts.size(); ++i)
                {
                    ia_vec.emplace_back();
                    ia_vec.back().binding = 0;
                    switch(info.vertexLayout.layouts[i].first)
                    {
                    case ResourceType::eInt32:
                        ia_vec.back().format = VK_FORMAT_R32_SINT;
                        break;
                    case ResourceType::eUint32:
                        ia_vec.back().format = VK_FORMAT_R32_UINT;
                        break;
                    case ResourceType::eFVec2:
                        ia_vec.back().format = VK_FORMAT_R32G32_SFLOAT;
                        break;
                    case ResourceType::eFVec3:
                        ia_vec.back().format = VK_FORMAT_R32G32B32_SFLOAT;
                        break;
                    case ResourceType::eFVec4:
                        ia_vec.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        break;
                    default:
                        std::cout << "Resource type (" << i << ") is not described\n";
                        return Result::eFailure;
                        break;
                    }
                }
            }

            //頂点まとめ
            VkPipelineVertexInputStateCreateInfo visci{};
            {
                visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                visci.pNext = nullptr;
                visci.vertexBindingDescriptionCount = 1;
                visci.pVertexBindingDescriptions = &ib;
                visci.vertexAttributeDescriptionCount = static_cast<uint32_t>(ia_vec.size());
                visci.pVertexAttributeDescriptions = ia_vec.data();
            }


            // ブレンディング
            VkPipelineColorBlendAttachmentState blendAttachment{};
            VkPipelineColorBlendStateCreateInfo cbci{};
            {
                const auto colorWriteAll =
                    VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT;
                switch(info.colorBlend)
                {
                case ColorBlend::eDefault:
                    blendAttachment.blendEnable = VK_TRUE;
                    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                    blendAttachment.colorWriteMask = colorWriteAll;

                    cbci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                    cbci.attachmentCount = 1;
                    cbci.pAttachments = &blendAttachment;
                    break;

                default:
                    std::cout << "color blend state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            // ビューポート, シザー
            VkPipelineViewportStateCreateInfo vpsci{};
            VkViewport viewport{};
            VkRect2D scissor{};
            
            {
                if(info.viewport)
                {
                    viewport.x = info.viewport.value()[0][0];
                    viewport.y = info.viewport.value()[0][1];
                    viewport.minDepth = info.viewport.value()[0][2];

                    viewport.width = info.viewport.value()[1][0];
                    viewport.height = -1.0f * info.viewport.value()[1][1];//上は反転します
                    viewport.maxDepth = info.viewport.value()[1][2];
                }
                else
                {
                    const VkExtent3D& extent = rdsto.mExtent.value();
                    viewport.x = 0;
                    viewport.y = -1.f * extent.height;
                    viewport.width = static_cast<float>(extent.width);
                    viewport.height = -1.f * static_cast<float>(extent.height);
                    viewport.minDepth = 0;
                    viewport.maxDepth = extent.depth;
                }

                if(info.scissor)
                {
                    scissor.offset.x = static_cast<float>(info.scissor.value()[0][0]);
                    scissor.offset.y = static_cast<float>(info.scissor.value()[0][1]);
                    scissor.extent =
                    {static_cast<uint32_t>(info.scissor.value()[1][0]), 
                    static_cast<uint32_t>(info.scissor.value()[1][1])};
                }
                else
                {
                    scissor.offset = {0, 0};
                    scissor.extent =
                        {rdsto.mExtent.value().width,
                        rdsto.mExtent.value().height};
                }

                vpsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                vpsci.viewportCount = 1;
                vpsci.pViewports = &viewport;
                vpsci.scissorCount = 1;
                vpsci.pScissors = &scissor;
            }

            //プリミティブトポロジー
            VkPipelineInputAssemblyStateCreateInfo iaci{};
            {
                iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                switch(info.topology)
                {
                case Topology::eTriangleList:
                    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    break;
                case Topology::eTriangleStrip:
                    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                    break;
                default:
                    std::cout << "primitive topology is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //ラスタライザ
            VkPipelineRasterizationStateCreateInfo rci{};
            {
                rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                switch(info.rasterizerState)
                {
                case RasterizerState::eDefault:
                    rci.polygonMode = VK_POLYGON_MODE_FILL;
                    rci.cullMode = VK_CULL_MODE_NONE;
                    rci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                    rci.lineWidth = 1.f;
                    break;
                default:
                    std::cout << "rasterizer state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }
            
            //マルチサンプルステート
            VkPipelineMultisampleStateCreateInfo msci{};
            {
                msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                switch(info.multiSampleState)
                {
                case MultiSampleState::eDefault:
                    msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                    break;
                default:
                    std::cout << "multisampling state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //デプスステンシルステート
            VkPipelineDepthStencilStateCreateInfo dsci{};
            {
                dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                switch(info.depthStencilState)
                {
                case DepthStencilState::eNone:
                    dsci.depthTestEnable = dsci.stencilTestEnable = VK_FALSE;
                    break;
                case DepthStencilState::eDepth:
                    dsci.depthTestEnable = VK_TRUE;
                    dsci.stencilTestEnable = VK_FALSE;
                    break;
                case DepthStencilState::eStencil:
                    dsci.depthTestEnable = VK_FALSE;
                    dsci.stencilTestEnable = VK_TRUE;
                    break;
                case DepthStencilState::eBoth:
                    dsci.depthTestEnable = dsci.stencilTestEnable = VK_TRUE;
                    break;
                default:
                    std::cout << "depth stencil state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }
            
            std::array<VkPipelineShaderStageCreateInfo, 2> ssciArr;

            result = createShaderModule(info.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, &ssciArr[0]);
            result = createShaderModule(info.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &ssciArr[1]);

            {

                VkDescriptorSetLayoutCreateInfo descLayoutci{};
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                rpo.mDescriptorCount.first = info.SRLayouts.uniformBufferCount;
                rpo.mDescriptorCount.second = info.SRLayouts.combinedTextureCount;

                for (int i = 0; i < info.SRLayouts.uniformBufferCount; ++i)
                {
                    bindings.emplace_back();
                    auto &b = bindings.back();

                    b.binding = i;
                    b.descriptorCount = 1;
                    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    b.stageFlags = VK_SHADER_STAGE_ALL;
                }

                for (int i = info.SRLayouts.uniformBufferCount; i < info.SRLayouts.uniformBufferCount + info.SRLayouts.combinedTextureCount; ++i)
                {
                    bindings.emplace_back();
                    auto &b = bindings.back();

                    b.binding = i;
                    b.descriptorCount = 1;
                    b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    b.stageFlags = VK_SHADER_STAGE_ALL;
                }

                descLayoutci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descLayoutci.bindingCount = bindings.size();
                descLayoutci.pBindings = bindings.data();

                {
                    VkDescriptorSetLayout descriptorSetLayout;
                    result = checkVkResult(vkCreateDescriptorSetLayout(mDevice, &descLayoutci, nullptr, &descriptorSetLayout));
                    if(result != Result::eSuccess)
                    {
                        std::cout << "failed to create descriptor set layout\n";
                        return result;
                    }

                    rpo.mDescriptorSetLayout = descriptorSetLayout;
                }
            }

            {//ディスクリプタプール構築
                std::array<VkDescriptorPoolSize, 2> sizes;
                sizes[0].descriptorCount = rpo.mDescriptorCount.first;
                sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sizes[1].descriptorCount = rpo.mDescriptorCount.second;
                sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

                VkDescriptorPoolCreateInfo dpci{};
                dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                if(info.maxSR < rpo.mDescriptorCount.first + rpo.mDescriptorCount.second)
                {
                    std::cout << "invalid max SR count\n";
                    return Result::eFailure;
                }
                dpci.maxSets = info.maxSR;
                dpci.poolSizeCount = sizes.size();
                dpci.pPoolSizes = sizes.data();
                {
                    VkDescriptorPool descriptorPool;
                    result = checkVkResult(vkCreateDescriptorPool(mDevice, &dpci, nullptr, &descriptorPool));
                    if (result != Result::eSuccess)
                    {
                        std::cout << "failed to create Descriptor Pool\n";
                        return result;
                    }
                    rpo.mDescriptorPool = descriptorPool;
                }
            }

            {//パイプラインレイアウト構築
                VkPipelineLayoutCreateInfo ci{};
                ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                ci.setLayoutCount = 1;
                VkDescriptorSetLayout dslayout = rpo.mDescriptorSetLayout.value();
                ci.pSetLayouts = &dslayout;

                {
                    VkPipelineLayout pipelineLayout;
                    result = checkVkResult(vkCreatePipelineLayout(mDevice, &ci, nullptr, &pipelineLayout));
                    if (result != Result::eSuccess)
                    {
                        std::cout << "failed to create pipeline layout\n";
                        return result;
                    }

                    rpo.mPipelineLayout = pipelineLayout;
                }
            }

            {
                VkGraphicsPipelineCreateInfo ci{};
                ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;                        
                ci.stageCount = static_cast<uint32_t>(ssciArr.size());
                ci.pStages = ssciArr.data();
                ci.pInputAssemblyState = &iaci;
                ci.pVertexInputState = &visci;
                ci.pRasterizationState = &rci;
                ci.pDepthStencilState = &dsci;
                ci.pMultisampleState = &msci;
                ci.pViewportState = &vpsci;
                ci.pColorBlendState = &cbci;
                ci.renderPass = rdsto.mRenderPass.value();
                ci.layout = rpo.mPipelineLayout.value();
                {
                    VkPipeline pipeline;
                    result = checkVkResult(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline));
                    if(result != Result::eSuccess)
                    {
                        std::cout << "failed to create graphics pipeline\n";
                        return result;
                    }

                    rpo.mPipeline = pipeline;
                }
            }
        }

        *pHandle = mNextRPHandle++;
        mRPMap.emplace(*pHandle, rpo);

        return Result::eSuccess;
    }

    Result Device::writeCommand(const Command& command)//ボトルネック
    {
        Result result = Result::eSuccess;

        {//optionalに構築したオブジェクトを代入している
            RunningCommandState tmp;
            mRCState = tmp;
        }


        switch (command.type)//型とenumの対応はめんどくさい
        {
        case CommandType::eCmdBeginRenderPipeline:
            // if (!std::holds_alternative<CmdBeginRenderPipeline>(command.info))
            //     return Result::eFailure;
            result = cmdBeginRenderPipeline(std::get<CmdBeginRenderPipeline>(command.info));
            break;
        case CommandType::eCmdEndRenderPipeline:
            // if (!std::holds_alternative<CmdEndRenderPipeline>(command.info))
            //     return Result::eFailure;
            result = cmdEndRenderPipeline(std::get<CmdEndRenderPipeline>(command.info));
            break;
        case CommandType::eCmdSetVB:
            // if (!std::holds_alternative<CmdSetVB>(command.info))
            //     return Result::eFailure;
            result = cmdSetVB(std::get<CmdSetVB>(command.info));
            break;
        case CommandType::eCmdSetIB:
            // if (!std::holds_alternative<CmdSetIB>(command.info))
            //     return Result::eFailure;
            result = cmdSetIB(std::get<CmdSetIB>(command.info));
            break;
        case CommandType::eCmdSetShaderResource:
            // if (!std::holds_alternative<CmdSetShaderResource>(command.info))
            //     return Result::eFailure;
            result = cmdSetShaderResource(std::get<CmdSetShaderResource>(command.info));
            break;
        case CommandType::eCmdRender:
            // if (!std::holds_alternative<CmdRender>(command.info))
            //     return Result::eFailure;
            result = cmdRender(std::get<CmdRender>(command.info));
            break;
        default:
            std::cout << "invalid command!\n";
            result = Result::eFailure;
            break;
        }

        return result;
    }


    Result Device::cmdBeginRenderPipeline(const CmdBeginRenderPipeline &info){
    {
        Result result = Result::eSuccess;
        if(mRPMap.count(info.RPHandle) <= 0)
        {
            std::cout << "invalid RP handle!";
            return Result::eFailure;
        }
        mRCState->mHRPO = info.RPHandle;
        auto& rpo = mRPMap[info.RPHandle];
        auto& rdsto = mRDSTMap[rpo.mHRenderDST];
        VkRenderPassBeginInfo bi{};

        if (rdsto.mHSwapchain)
        {
            auto& swapchain = mSwapchainMap[rdsto.mHSwapchain.value()];
                result = checkVkResult(vkAcquireNextImageKHR(mDevice, swapchain.mSwapchain.value(), UINT64_MAX, mPresentCompletedSem, VK_NULL_HANDLE, &mFrameIndex));
                if (result != Result::eSuccess)
                {
                    std::cout << "failed to acquire next swapchain image!\n";
                    return result;
                }

                //if (command.clearValue.size() != rdsto.mTargetnum)
                //{
                //    std::cout << "invalid clear value!\n";
                //}

                // クリア値
                std::vector<VkClearValue> clearValue;
                clearValue.emplace_back(VkClearValue{ 0.5f, 0.5f, 0.5f, 0.f });
                if (rdsto.mTargetNum > 1)
                {
                    clearValue.emplace_back(VkClearValue{ 1.0f, 0 });
                }

                bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                bi.renderPass = rdsto.mRenderPass.value();
                bi.framebuffer = rdsto.mFramebuffers[mFrameIndex].value();
                bi.renderArea.offset = VkOffset2D{ 0, 0 };
                bi.renderArea.extent = swapchain.mSwapchainExtent;
                bi.clearValueCount = clearValue.size();
                bi.pClearValues = clearValue.data();
            }
            else
            {
                bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                bi.renderPass = rdsto.mRenderPass.value();
                bi.framebuffer = rdsto.mFramebuffers.back().value();
                bi.renderArea.offset = VkOffset2D{ 0, 0 };
                bi.renderArea.extent = VkExtent2D{ rdsto.mExtent.value().width, rdsto.mExtent.value().height};
            }

            auto& vkcommand = mCommands[mFrameIndex];
            auto commandFence = mFences[mFrameIndex];
            vkWaitForFences(mDevice, 1, &commandFence, VK_TRUE, UINT64_MAX);

            VkCommandBufferBeginInfo commandBI{};
            commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            vkBeginCommandBuffer(mCommands[mFrameIndex], &commandBI);
        }
         return Result::eSuccess;
    }
    Result Device::cmdEndRenderPipeline(const CmdEndRenderPipeline &info)
    {
        Result result;
        // コマンド・レンダーパス終了
        vkCmdEndRenderPass(mCommands[mFrameIndex]);
        result = checkVkResult(vkEndCommandBuffer(mCommands[mFrameIndex]));
        if(result != Result::eSuccess)
            return result;
        
        return Result::eSuccess;
    }
    Result Device::cmdSetVB(const CmdSetVB &info)
    {
        if(mBufferMap.count(info.VBHandle) <= 0)
            return Result::eFailure;
        auto &vbo = mBufferMap[info.VBHandle];
        vkCmdBindVertexBuffers(mCommands[mFrameIndex], 0, 1, &vbo.mBuffer.value(), nullptr);
        return Result::eSuccess;
    }
    Result Device::cmdSetIB(const CmdSetIB &info)
    {
        if (mBufferMap.count(info.IBHandle) <= 0)
            return Result::eFailure;
        auto &ibo = mBufferMap[info.IBHandle];
        vkCmdBindIndexBuffer(mCommands[mFrameIndex], ibo.mBuffer.value(), 0, VK_INDEX_TYPE_UINT32);
        return Result::eSuccess;
    }
    Result Device::cmdSetShaderResource(const CmdSetShaderResource &info)
    {
        Result result;

        if(!mRCState->mHRPO)
        {
            std::cout << "render pipeline object is not registed yet!\n";
            return Result::eFailure;
        }

        auto &rpo = mRPMap[mRCState->mHRPO.value()];

        VkDescriptorSet descriptorSet;
        {
            VkDescriptorSetAllocateInfo dsai{};
            dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsai.descriptorPool = rpo.mDescriptorPool.value();
            dsai.descriptorSetCount = 1;
            dsai.pSetLayouts = &rpo.mDescriptorSetLayout.value();
            result = checkVkResult(vkAllocateDescriptorSets(mDevice, &dsai, &descriptorSet));
            if (result != Result::eSuccess)
            {
                std::cout << "failed to allocate descriptor set\n";
                return result;
            }

            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.reserve(rpo.mDescriptorCount.first + rpo.mDescriptorCount.second);

            uint32_t count = 0;
            for (auto& hub :info.shaderResource.uniformBuffer)
            {
                VkDescriptorBufferInfo dbi;

                if (mBufferMap.count(hub) <= 0)
                {
                    std::cout << "invalid uniform buffer handle!\n";
                    return Result::eFailure;
                }

                auto& ub = mBufferMap[hub];

                dbi.buffer = ub.mBuffer.value();
                dbi.offset = 0;
                dbi.range = VK_WHOLE_SIZE;

                writeSets.emplace_back(VkWriteDescriptorSet{});
                writeSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeSets.back().dstBinding = count++;
                writeSets.back().descriptorCount = 1;
                writeSets.back().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeSets.back().pBufferInfo = &dbi;
                writeSets.back().dstSet = descriptorSet;
            }

            count = 0;
            for (auto& hct : info.shaderResource.combinedTexture)
            {
                VkDescriptorImageInfo dii;

                if (mImageMap.count(hct) <= 0)
                {
                    std::cout << "invalid combined texture handle!\n";
                    return Result::eFailure;
                }

                auto& ct = mImageMap[hct];
                dii.imageView = ct.mView.value();
                dii.sampler = ct.mSampler.value();
                dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                writeSets.emplace_back(VkWriteDescriptorSet{});
                writeSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeSets.back().dstBinding = count++;
                writeSets.back().descriptorCount = 1;
                writeSets.back().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeSets.back().pImageInfo = &dii;
                writeSets.back().dstSet = descriptorSet;
            }

            vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
        }

        mRCState->mDescriptorSet = descriptorSet;

        vkCmdBindDescriptorSets
        (
            mCommands[mFrameIndex], 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            rpo.mPipelineLayout.value(), 
            0, 
            1, 
            &descriptorSet, 
            0, 
            nullptr
        );

        return Result::eSuccess;
    }
    Result Device::cmdRender(const CmdRender &info)
    {
        vkCmdDrawIndexed
        (
            mCommands[mFrameIndex], 
            info.indexCount, 
            info.instanceCount, 
            info.firstIndex, 
            info.vertexOffset, 
            info.firstInstance
        );
        return Result::eSuccess;
    }

    Result Device::execute()
    {
        Result result;
        // コマンドを実行（送信)
        VkSubmitInfo submitInfo{};
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommands[mFrameIndex];
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &mPresentCompletedSem;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &mRenderCompletedSem;

        result = checkVkResult(vkResetFences(mDevice, 1, &mFences[mFrameIndex]));
        if(result != Result::eSuccess)
        {
            std::cout << "failed to reset fences!\n";
            return result;
        }

        result = checkVkResult(vkQueueSubmit(mDeviceQueue, 1, &submitInfo, mFences[mFrameIndex]));
        if(result != Result::eSuccess)
        {
            std::cout << "failed to submit cmd to queue!\n";
            return result;
        }

        //注意 : VkDescriptorSetについての解放がよくわかっていない
        mRCState = std::nullopt;

        return Result::eSuccess;
    }

    Result Device::present(const HSwapchain& handle)
    {
        Result result;

        if(mSwapchainMap.count(handle) <= 0)
        {
            std::cout << "invalid swapchian handle!\n";
            return Result::eFailure;
        }

        auto& so = mSwapchainMap[handle];

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &so.mSwapchain.value();
        presentInfo.pImageIndices = &mFrameIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderCompletedSem;
        
        result = checkVkResult(vkQueuePresentKHR(mDeviceQueue, &presentInfo));
        if (result != Result::eSuccess)
        {
            std::cout << "failed to present queue!\n";
            return result;
        }

        mFrameIndex = (mFrameIndex + 1) % so.mHSwapchainImages.size();

        return Result::eSuccess;
    }

    //  Result result;

    //     if (mRPMap.count(handle) <= 0)
    //     {
    //         std::cout << "invalid renderPipeline handle!\n";
    //         return Result::eFailure;
    //     }
    //     auto &rpo = mRPMap[handle];
        
    //     VkDescriptorSet descriptorSet;
    //     {
    //         VkDescriptorSetAllocateInfo dsai{};
    //         dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //         dsai.descriptorPool = 
    //         dsai.descriptorSetCount = 1;
    //         dsai.pSetLayouts = &rpo.mDescriptorSetLayout.value();
    //         result = checkVkResult(vkAllocateDescriptorSets(mDevice, &dsai, &descriptorSet));
    //         if (result != Result::eSuccess)
    //         {
    //             std::cout << "failed to allocate descriptor set\n";
    //             return result;
    //         }

    //         std::vector<VkWriteDescriptorSet> writeSets;
    //         writeSets.reserve(rpo.mDescriptorCount.first + rpo.mDescriptorCount.second);

    //         uint32_t count = 0;
    //         for (auto& hub : command.shaderResource.uniformBuffer)
    //         {
    //             VkDescriptorBufferInfo dbi;

    //             if (mBufferMap.count(hub) <= 0)
    //             {
    //                 std::cout << "invalid uniform buffer handle!\n";
    //                 return Result::eFailure;
    //             }

    //             auto& ub = mBufferMap[hub];

    //             dbi.buffer = ub.mBuffer.value();
    //             dbi.offset = 0;
    //             dbi.range = VK_WHOLE_SIZE;

    //             writeSets.emplace_back(VkWriteDescriptorSet{});
    //             writeSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //             writeSets.back().dstBinding = count++;
    //             writeSets.back().descriptorCount = 1;
    //             writeSets.back().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //             writeSets.back().pBufferInfo = &dbi;
    //             writeSets.back().dstSet = descriptorSet;
    //         }

    //         count = 0;
    //         for (auto& hct : command.shaderResource.combinedTexture)
    //         {
    //             VkDescriptorImageInfo dii;

    //             if (mImageMap.count(hct) <= 0)
    //             {
    //                 std::cout << "invalid combined texture handle!\n";
    //                 return Result::eFailure;
    //             }

    //             auto& ct = mImageMap[hct];
    //             dii.imageView = ct.mView.value();
    //             dii.sampler = ct.mSampler.value();
    //             dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    //             writeSets.emplace_back(VkWriteDescriptorSet{});
    //             writeSets.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //             writeSets.back().dstBinding = count++;
    //             writeSets.back().descriptorCount = 1;
    //             writeSets.back().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //             writeSets.back().pImageInfo = &dii;
    //             writeSets.back().dstSet = descriptorSet;
    //         }

    //         vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    //     }

    //     {
    //         auto& rdsto = mRDSTMap[rpo.mHRenderDST];
    //         VkRenderPassBeginInfo bi{};

    //         if (rdsto.mHSwapchain)
    //         {
    //             auto& swapchain = mSwapchainMap[rdsto.mHSwapchain.value()];
    //             result = checkVkResult(vkAcquireNextImageKHR(mDevice, swapchain.mSwapchain.value(), UINT64_MAX, mPresentCompletedSem, VK_NULL_HANDLE, &mFrameIndex));
    //             if (result != Result::eSuccess)
    //             {
    //                 std::cout << "failed to acquire next swapchain image!\n";
    //                 return result;
    //             }

    //             //if (command.clearValue.size() != rdsto.mTargetnum)
    //             //{
    //             //    std::cout << "invalid clear value!\n";
    //             //}

    //             // クリア値
    //             std::vector<VkClearValue> clearValue;
    //             clearValue.emplace_back(VkClearValue{ 0.5f, 0.5f, 0.5f, 0.f });
    //             if (rdsto.mTargetNum > 1)
    //             {
    //                 clearValue.emplace_back(VkClearValue{ 1.0f, 0 });
    //             }

    //             bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //             bi.renderPass = rdsto.mRenderPass.value();
    //             bi.framebuffer = rdsto.mFramebuffers[mFrameIndex].value();
    //             bi.renderArea.offset = VkOffset2D{ 0, 0 };
    //             bi.renderArea.extent = swapchain.mSwapchainExtent;
    //             bi.clearValueCount = clearValue.size();
    //             bi.pClearValues = clearValue.data();
    //         }
    //         else
    //         {
    //             bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //             bi.renderPass = rdsto.mRenderPass.value();
    //             bi.framebuffer = rdsto.mFramebuffers.back().value();
    //             bi.renderArea.offset = VkOffset2D{ 0, 0 };
    //             bi.renderArea.extent = VkExtent2D{ rdsto.mExtent.value().width, rdsto.mExtent.value().height};
    //         }

    //         auto& vkcommand = mCommands[mFrameIndex];
    //         auto commandFence = mFences[mFrameIndex];
    //         vkWaitForFences(mDevice, 1, &commandFence, VK_TRUE, UINT64_MAX);

    //     }

    //if (mBufferMap.count(command.hVertexBuffer.value()) <= 0)
    //{
    //    std::cout << "invalid Buffer handle!\n";
    //    return Result::eFailure;
    //}
    //auto& vbo = mBufferMap[command.hVertexBuffer.value()];

    //if (mBufferMap.count(command.hIndexBuffer.value()) <= 0)
    //{
    //    std::cout << "invalid Buffer handle!\n";
    //    return Result::eFailure;
    //}
    //auto ibo = mBufferMap[command.hIndexBuffer.value()];

};     