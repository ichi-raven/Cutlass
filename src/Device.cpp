#include "../include/Device.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <variant>
#include <memory>

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
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        VkBool32 ret = VK_FALSE;
        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ||
            flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            ret = VK_TRUE;
        }

        if (pLayerPrefix)
        {
            std::cerr << "[" << pLayerPrefix << "] ";
        }
        std::cerr << pMessage << std::endl;

        return ret;
    }

    Result Device::initialize(const InitializeInfo& initializeInfo, std::vector<HSwapchain>& handlesRef)
    {
        mInitializeInfo = initializeInfo;
        Result result;
        mCurrentFrame = 0;

        std::cerr << "initialize started...\n";

        //GLFW初期化

        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW!\n";
            exit(-1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        std::cerr << "GLFW initialized\n";

        //インスタンス作成
        result = createInstance();
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::cerr << "created VkInstance\n";

        if (mInitializeInfo.debugFlag)
        {
            result = enableDebugReport();
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        //物理デバイス選択(いまのところ特に指定なし、先頭デバイスを使用する)
        result = selectPhysicalDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "selected Physical Device\n";

        //グラフィクスキューのインデックス検索(いまのところ特に条件指定はない)
        result = searchGraphicsQueueIndex();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "found index of GraphicsQueue\n";

        //論理デバイス作成
        result = createDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkDevice\n";

        //コマンドプール作成
        result = createCommandPool();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkCommandPool\n";

        //ウィンドウサーフェス、スワップチェイン作成
        {
            int time = 0;
            for (auto& w : mInitializeInfo.windows)
            {
                std::cerr << "Swapchain No. " << time++ << "------\n";
                SwapchainObject so;

                so.mpWindow = std::make_optional(glfwCreateWindow(
                    w.width,
                    w.height,
                    w.windowName.c_str(),
                    nullptr,
                    nullptr));

                if (!so.mpWindow.value())
                {
                    std::cerr << "Failed to create GLFW Window!\n";
                    destroy();
                    exit(-1);
                }

                result = createSurface(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cerr << "created VkSurfaceKHR\n";

                result = createSwapchain(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cerr << "created VkSwapChainKHR\n";

                result = createSwapchainImages(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cerr << "created swapchain images\n";

                result = createDepthBuffer(&so);
                if (Result::eSuccess != result)
                {
                    return result;
                }
                std::cerr << "created swapchain depthbuffer\n";

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
        std::cerr << "created VkSemaphore\n";

        std::cerr << "all initialize processes succeeded\n";
        return Result::eSuccess;
    }

    Result Device::destroy()
    {
        std::cerr << "destroying...\n";

        if (VK_SUCCESS != vkDeviceWaitIdle(mDevice))
            std::cerr << "Failed to wait device idol\n";

        for (auto& e : mBufferMap)
        {
            if (e.second.mBuffer)
                vkDestroyBuffer(mDevice, e.second.mBuffer.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);
        }
        mBufferMap.clear();
        std::cerr << "destroyed user allocated buffers\n";

        for (auto& e : mImageMap)
        {
            if (e.second.mView)
                vkDestroyImageView(mDevice, e.second.mView.value(), nullptr);

            if (e.second.usage == TextureUsage::eSwapchainImage)//SwapchainImageはここだけを破棄する
                continue;

            if (e.second.mImage)
                vkDestroyImage(mDevice, e.second.mImage.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);

            //if(e.second.mFrameBuffer)
            //    vkDestroyFramebuffer(mDevice, e.second.mFrameBuffer.value(), nullptr);

            if (e.second.mSampler)
                vkDestroySampler(mDevice, e.second.mSampler.value(), nullptr);
        }
        mImageMap.clear();
        std::cerr << "destroyed user allocated textures\n";
        std::cerr << "destroyed user allocated sampler\n";

        for (auto& e : mRDSTMap)
        {
            for (auto& f : e.second.mFramebuffers)
                vkDestroyFramebuffer(mDevice, f.value(), nullptr);

            if (e.second.mRenderPass)
                vkDestroyRenderPass(mDevice, e.second.mRenderPass.value(), nullptr);
        }

        for (auto& e : mRPMap)
        {
            if (e.second.mDescriptorSetLayout)
                vkDestroyDescriptorSetLayout(mDevice, e.second.mDescriptorSetLayout.value(), nullptr);
            if (e.second.mDescriptorPool)
                vkDestroyDescriptorPool(mDevice, e.second.mDescriptorPool.value(), nullptr);
            if (e.second.mPipelineLayout)
                vkDestroyPipelineLayout(mDevice, e.second.mPipelineLayout.value(), nullptr);
            if (e.second.mPipeline)
                vkDestroyPipeline(mDevice, e.second.mPipeline.value(), nullptr);
        }

        for (auto& e : mCommandBufferMap)
        {
            vkFreeCommandBuffers(mDevice, mCommandPool, uint32_t(e.second.mCommandBuffers.size()), e.second.mCommandBuffers.data());
            //e.second.mCommandBuffers.clear();
            for (auto& f : e.second.mFences)
                vkDestroyFence(mDevice, f, nullptr);
        }

        std::cerr << "destroyed fences\n";
        std::cerr << "destroyed command buffers\n";
        //for(auto& e : mSamplerMap)
        //{
        //    vkDestroySampler(mDevice, e.second, nullptr);
        //}
        //mSamplerMap.clear();


        // for (auto &v : mFences)
        // {
        //     vkDestroyFence(mDevice, v, nullptr);
        // }
        //mFences.clear();

        for (const auto& pcSem : mPresentCompletedSems)
            vkDestroySemaphore(mDevice, pcSem, nullptr);
        for (const auto& rcSem : mRenderCompletedSems)
            vkDestroySemaphore(mDevice, rcSem, nullptr);
        std::cerr << "destroyed semaphores\n";

        // if(mCommands.size() > 0)//現状こうしてある
        // {
        //     vkFreeCommandBuffers(mDevice, mCommandPool, uint32_t(mCommands.size()), mCommands.data());
        //     mCommands.clear();

        // }

        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        std::cerr << "destroyed command pool\n";

        for (auto& so : mSwapchainMap)
        {
            //std::cerr << "destroyed swapchain imageViews\n";
            if (so.second.mSwapchain)
            {
                vkDestroySwapchainKHR(mDevice, so.second.mSwapchain.value(), nullptr);
                std::cerr << "destroyed swapchain\n";
            }

            if (so.second.mSurface)
            {
                vkDestroySurfaceKHR(mInstance, so.second.mSurface.value(), nullptr);
                std::cerr << "destroyed surface\n";
            }

            if (so.second.mpWindow)
            {
                glfwDestroyWindow(so.second.mpWindow.value());
                std::cerr << "destroyed GLFW window\n";
            }
        }

        mSwapchainMap.clear();
        std::cerr << "destroyed all swapchain\n";

        if (mInitializeInfo.debugFlag)
            disableDebugReport();

        vkDestroyDevice(mDevice, nullptr);
        std::cerr << "destroyed device\n";

        vkDestroyInstance(mInstance, nullptr);
        std::cerr << "destroyed instance\n";

        glfwTerminate();
        std::cerr << "GLFW Terminated\n";

        std::cerr << "all destroying process succeeded\n";

        return Result::eSuccess;
    }

    Result Device::checkVkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cerr << "ERROR!\nvulkan error : " << result << "\n";
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

        if (mInitializeInfo.debugFlag)
        {
            // デバッグ時には検証レイヤーを有効化
            const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
            //const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
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

        std::cerr << "Physical Device number : " << physDevs.size() << "\n";

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

            std::vector<const char*> extensions;
            for (const auto& v : devExtProps)
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

        {
            VkCommandPoolCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.queueFamilyIndex = mGraphicsQueueIndex;
            ci.flags = 0;

            result = checkVkResult(vkCreateCommandPool(mDevice, &ci, nullptr, &mCommandPool));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }
        return Result::eSuccess;
    }

    Result Device::createSurface(SwapchainObject* pSO)
    {
        Result result;

        VkSurfaceKHR surface;
        result = checkVkResult(glfwCreateWindowSurface(mInstance, pSO->mpWindow.value(), nullptr, &surface));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to create window surface!\n";
            return result;
        }
        pSO->mSurface = surface;

        // サーフェスのフォーマット情報選択
        result = selectSurfaceFormat(pSO, VK_FORMAT_B8G8R8A8_UNORM);
        if (Result::eSuccess != result)
        {
            std::cout << "Failed to select surface format!\n";
            return result;
        }

        // サーフェスの能力値情報取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysDev, pSO->mSurface.value(), &pSO->mSurfaceCaps));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to get physical device surface capability!\n";
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

        uint32_t surfaceFormatCount = 0; //個数取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, pSO->mSurface.value(), &surfaceFormatCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount); //実際のフォーマット取得
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, pSO->mSurface.value(), &surfaceFormatCount, formats.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        //一致するフォーマットを検索
        for (const auto& f : formats)
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

        mMaxFrameNum = std::max(pSO->mSurfaceCaps.minImageCount, mInitializeInfo.frameCount);
        mMaxFrameInFlight = mMaxFrameNum - 1;

        auto &extent = pSO->mSurfaceCaps.currentExtent;
        if (extent.width <= 0u || extent.height <= 0u)
        {
            // 値が無効なのでウィンドウサイズを使用する
            int width, height;
            glfwGetWindowSize(pSO->mpWindow.value(), &width, &height);
            extent.width = static_cast<uint32_t>(width);
            extent.height = static_cast <uint32_t>(height);
        }

        pSO->mPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t queueFamilyIndices[] = { mGraphicsQueueIndex };
        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = pSO->mSurface.value();
        ci.minImageCount = mMaxFrameNum;
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
                std::cerr << "Failed to create Swapchain!\n";
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
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            };
            ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            ci.image = swapchainImages[i];
            result = checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &swapchainViews[i]));
            if (Result::eSuccess != result)
            {
                std::cerr << "failed to create swapchain image views!\n";
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
            io.extent = { pSO->mSwapchainExtent.width, pSO->mSwapchainExtent.height, static_cast<uint32_t>(1) };
            io.format = pSO->mSurfaceFormat.format;

            pSO->mHSwapchainImages.emplace_back(mNextTextureHandle++);
            mImageMap.emplace(pSO->mHSwapchainImages.back(), io);
        }

        return Result::eSuccess;
    }

    Result Device::createSemaphores()
    {
        Result result = Result::eSuccess;

        VkSemaphoreCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        mPresentCompletedSems.resize(mMaxFrameInFlight);
        mRenderCompletedSems.resize(mMaxFrameInFlight);

        for (size_t i = 0; i < mMaxFrameInFlight; ++i)
        {
            //mRenderCompletedSems.emplace_back();
            result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mRenderCompletedSems[i]));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create render completed semaphore!\n";
                return result;
            }
        }

        for (size_t i = 0; i < mMaxFrameInFlight; ++i)
        {
            result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mPresentCompletedSems[i]));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create present completed semaphore!\n";
                return result;
            }
        }

        imagesInFlight.resize(mMaxFrameNum, VK_NULL_HANDLE);

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

        GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
        GetInstanceProcAddr(vkDebugReportMessageEXT);
        GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

        VkDebugReportFlagsEXT flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT drcci{};
        drcci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drcci.flags = flags;
        drcci.pfnCallback = &DebugReportCallback;
        mvkCreateDebugReportCallbackEXT(mInstance, &drcci, nullptr, &mDebugReport);

        std::cerr << "enabled debug mode\n";

        return Result::eSuccess;
    }

    Result Device::disableDebugReport()
    {

        if (mvkDestroyDebugReportCallbackEXT)
        {
            mvkDestroyDebugReportCallbackEXT(mInstance, mDebugReport, nullptr);
        }

        std::cerr << "disabled debug mode\n";

        return Result::eSuccess;
    }

    uint32_t Device::getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
    {
        uint32_t result = 0;
        for (uint32_t i = 0; i < mPhysMemProps.memoryTypeCount; ++i)
        {
            if (requestBits & 1)
            {
                const auto& types = mPhysMemProps.memoryTypes[i];
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
    Result Device::createBuffer(const BufferInfo& info, HBuffer* pHandle)
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
                std::cerr << "buffer usage is not descibed.\n";
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

    Result Device::writeBuffer(const size_t size, const void* const pData, const HBuffer& handle)
    {
        Result result = Result::eFailure;
        if (mBufferMap.count(handle) <= 0)
            return result;

        BufferObject& bo = mBufferMap[handle];

        void* p;//マップ先アドレス

        result = checkVkResult(vkMapMemory(mDevice, bo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
        if (result != Result::eSuccess)
            return result;
        memcpy(p, pData, size);
        vkUnmapMemory(mDevice, bo.mMemory.value());

        return Result::eSuccess;
    }

    Result Device::createTexture(const TextureInfo& info, HTexture* pHandle)
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
                    std::cerr << "invalid type of pixel!\n";
                    return Result::eFailure;
                    break;
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
                    std::cerr << "invalid type of pixel!\n";
                    return Result::eFailure;
                    break;
                }
                break;
            default:
                std::cerr << "invalid type of pixel!\n";
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
                    std::cerr << "ignored invalid depth param\n";
                break;
                // case TextureDimention::e3D:
                //     ci.imageType = VK_IMAGE_TYPE_3D;
                //     ci.extent = {uint32_t(info.width), uint32_t(info.height), uint32_t(info.depth)};
                //     break;
            default:
                std::cerr << "invalid dimention of texture!\n";
                return Result::eFailure;
                break;
            }

            switch (info.usage)
            {
            case TextureUsage::eShaderResource:
                ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            case TextureUsage::eColorTarget:
                ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                ci.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;
            case TextureUsage::eDepthStencilTarget:
                ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                //ci.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
                break;
                // case TextureUsage::eUnordered: //不定なのでこまった
                //     ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                //     break;
            default:
                std::cerr << "invalid usage!\n";
                return Result::eFailure;
                break;
            }

            io.usage = info.usage;

            ci.arrayLayers = 1;
            ci.mipLevels = 1;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
                result = checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &imageView));
                if (result != Result::eSuccess)
                {
                    std::cerr << "failed to create vkImageView!\n";
                    return result;
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

        stbi_uc* pImage = nullptr;
        int width = 0, height = 0, channel = 0;
        io.format = VK_FORMAT_R8G8B8A8_UNORM;//固定する
        io.usage = TextureUsage::eShaderResource;

        {
            VkImageCreateInfo ci{};

            if (!fileName)
            {
                std::cerr << "invalid file name\n";
                return Result::eFailure;
            }

            pImage = stbi_load(fileName, &width, &height, nullptr, 4);
            channel = 4;
            if (!pImage)
            {
                std::cerr << "Failed to load texture file!\n";
                return Result::eFailure;
            }

            io.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(1) };

            // テクスチャのVkImageを生成
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.extent = { uint32_t(width), uint32_t(height), 1 };
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

    Result Device::writeTexture(const void* const pData, const HTexture& handle)
    {
        Result result = Result::eFailure;

        if (mImageMap.count(handle) <= 0)
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

            void* p;
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
        copyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
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
        imb.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
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

    /*Result Device::createCommandBuffers()
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
            std::cerr << "Failed to allocate command buffers!\n";
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
                std::cerr << "Failed to create fence!\n";
                return result;
            }
        }

        return Result::eSuccess;
    }*/


    Result Device::createDepthBuffer(SwapchainObject* pSO)
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
                    std::cerr << "failed to create depth buffer's image\n";
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
                std::cerr << "failed to create vkImageView!\n";
                return result;
            }

            io.mView = imageView;
        }

        io.usage = TextureUsage::eDepthStencilTarget;
        mImageMap.emplace(mNextTextureHandle, io);
        pSO->mHDepthBuffer = mNextTextureHandle++;

        return Result::eSuccess;
    }

    Result Device::createShaderModule(const Shader& shader, const VkShaderStageFlagBits& stage, VkPipelineShaderStageCreateInfo* pSSCI)
    {
        Result result;

        std::ifstream infile(shader.path.c_str(), std::ios::binary);
        if (!infile)
        {
            std::cerr << "failed to load file from " << shader.path << " \n";
            return Result::eFailure;
        }

        std::vector<char> filedata;
        filedata.resize(uint32_t(infile.seekg(0, std::ifstream::end).tellg()));
        infile.seekg(0, std::ifstream::beg).read(filedata.data(), filedata.size());

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.pCode = reinterpret_cast<uint32_t*>(filedata.data());
        ci.codeSize = filedata.size();
        result = checkVkResult(vkCreateShaderModule(mDevice, &ci, nullptr, &shaderModule));
        if (result != Result::eSuccess)
        {
            std::cerr << "failed to create Shader Module\n";
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


    Result Device::createRenderDSTFromSwapchain(const HSwapchain& handle, bool depthTestEnable, HRenderDST* pHandle)
    {
        Result result;
        RenderDSTObject rdsto;

        //フラグはさっさと代入
        rdsto.mDepthTestEnable = depthTestEnable;

        //Renderpass, 対象に応じたFramebuffer構築

        if (mSwapchainMap.count(handle) <= 0)
        {
            std::cerr << "invalid swapchain handle\n";
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
            std::vector<VkAttachmentDescription> adVec;
            VkAttachmentReference ar;
            VkAttachmentReference depthAr;

            //adVec.reserve(2);

            const auto& rdst = swapchain.mHSwapchainImages[0]; //情報得るだけなのでどれでもいい

            if (mImageMap.count(rdst) <= 0)
            {
                std::cerr << "invalid swapchain image handle\n";
                return Result::eFailure;
            }

            auto& io = mImageMap[rdst];
            adVec.emplace_back();
            adVec.back().format = io.format;
            adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
            adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            adVec.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            ar.attachment = 0;
            ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassDesc{};
            subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.pColorAttachments = &ar;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            //指定されていれば末尾にDSBを追加
            if (depthTestEnable)
            {
                adVec.emplace_back();

                auto& depthBuffer = mImageMap[swapchain.mHDepthBuffer];
                adVec.back().format = depthBuffer.format;
                adVec.back().samples = VK_SAMPLE_COUNT_1_BIT;
                adVec.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                adVec.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

            ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            ci.attachmentCount = adVec.size();
            ci.pAttachments = adVec.data();
            ci.subpassCount = 1;
            ci.pSubpasses = &subpassDesc;
            ci.dependencyCount = 1;
            ci.pDependencies = &dependency;
            {
                VkRenderPass renderPass;
                result = checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                rdsto.mRenderPass = renderPass;
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
            const auto& depth = mImageMap[swapchain.mHDepthBuffer];
            for (auto& h : swapchain.mHSwapchainImages)
            {

                const auto& img = mImageMap[h];
                std::array<VkImageView, 2> ivArr = { img.mView.value(), depth.mView.value() };
                fbci.attachmentCount = 2;
                fbci.pAttachments = ivArr.data();

                VkFramebuffer framebuffer;
                result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &framebuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to create frame buffer!\n";
                    return result;
                }
                rdsto.mFramebuffers.emplace_back(framebuffer);
                
            }
        }
        else
        {
            fbci.attachmentCount = 1;
            for (auto& h : swapchain.mHSwapchainImages)
            {

                //const auto& img = mImageMap[h];
                //VkImageView iv = img.mView.value();
                fbci.pAttachments = &mImageMap[h].mView.value();

                VkFramebuffer frameBuffer;
                result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to create framebuffer!\n";
                    return result;
                }

                rdsto.mFramebuffers.emplace_back(frameBuffer);
                //rdsto.mFramebuffers.back() = frameBuffer;

            }
        }

        *pHandle = mNextRenderDSTHandle++;
        mRDSTMap.emplace(*pHandle, rdsto);

        return Result::eSuccess;
    }

    Result Device::createRenderDSTFromTextures(const std::vector<HTexture> textures, HRenderDST* pHandle)
    {
        Result result;
        RenderDSTObject rdsto;

        rdsto.mHSwapchain = std::nullopt;

        VkRenderPassCreateInfo ci{};
        std::vector<VkAttachmentDescription> adVec;
        std::vector<VkAttachmentReference> arVec;
        std::optional<HTexture> hDepthBuffer;

        if (mInitializeInfo.debugFlag)//チェック
        {
            for (auto& tex : textures)
            {
                if (mImageMap.count(tex) <= 0)
                {
                    std::cerr << "invalid texture handle\n";
                    return Result::eFailure;
                }
                auto& io = mImageMap[tex];

                //使いみちが違うのはアウト
                if (io.usage != TextureUsage::eColorTarget || io.usage != TextureUsage::eDepthStencilTarget)
                {
                    std::cerr << "invalid texture usage\n";
                    return Result::eFailure;
                }

                //サイズが違うのもアウト
                if (!rdsto.mExtent)
                    rdsto.mExtent = io.extent;
                if (rdsto.mExtent.value().width != io.extent.width
                    && rdsto.mExtent.value().height != io.extent.height
                    && rdsto.mExtent.value().depth != io.extent.depth)
                {
                    std::cerr << "invalid texture extent\n";
                    return Result::eFailure;
                }
            }
        }

        //Renderpass, 対象に応じたFramebuffer構築
        for (auto& tex : textures)
        {

            auto& io = mImageMap[tex];
            adVec.emplace_back();
            arVec.emplace_back();

            if (!rdsto.mExtent)
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
            auto& depthBuffer = mImageMap[hDepthBuffer.value()];

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
        for (auto& tex : textures)
        {
            const auto& img = mImageMap[tex];
            ivVec.emplace_back(img.mView.value());
        }

        fbci.pAttachments = ivVec.data();
        rdsto.mFramebuffers.emplace_back();
        result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &rdsto.mFramebuffers.back().value()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        *pHandle = mNextRenderDSTHandle++;
        mRDSTMap.emplace(*pHandle, rdsto);

        return Result::eSuccess;
    }

    Result Device::createRenderPipeline(const RenderPipelineInfo& info, HRenderPipeline* pHandle)
    {
        Result result;
        RenderPipelineObject rpo;

        if (mRDSTMap.count(info.renderDST) <= 0)
        {
            std::cerr << "invalid renderDST handle!\n";
            return Result::eFailure;
        }

        RenderDSTObject rdsto = mRDSTMap[info.renderDST];
        rpo.mHRenderDST = info.renderDST;

        //PSO構築
        {
            //頂点バインディング
            VkVertexInputBindingDescription ib;
            //頂点属性
            std::vector<VkVertexInputAttributeDescription> ia_vec;
            //頂点まとめ
            VkPipelineVertexInputStateCreateInfo visci{};
            if (info.vertexLayout)
            {
                {
                    ib.binding = 0;
                    ib.stride = info.vertexLayout.value().sizeOfType;
                    ib.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                }

                {
                    if (info.vertexLayout.value().layouts.size() <= 0)
                    {
                        return Result::eFailure;
                    }

                    for (size_t i = 0; i < info.vertexLayout.value().layouts.size(); ++i)
                    {
                        ia_vec.emplace_back();
                        ia_vec.back().binding = 0;
                        switch (info.vertexLayout.value().layouts[i].first)
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
                            std::cerr << "Resource type (" << i << ") is not described\n";
                            return Result::eFailure;
                            break;
                        }
                    }
                }

                {
                    visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                    visci.pNext = nullptr;
                    visci.vertexBindingDescriptionCount = 1;
                    visci.pVertexBindingDescriptions = &ib;
                    visci.vertexAttributeDescriptionCount = static_cast<uint32_t>(ia_vec.size());
                    visci.pVertexAttributeDescriptions = ia_vec.data();
                }
            }
            else//頂点構造がない場合
            {

                visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                visci.pNext = nullptr;
                visci.vertexBindingDescriptionCount = 0;
                visci.pVertexBindingDescriptions = nullptr;
                visci.vertexAttributeDescriptionCount = 0;
                visci.pVertexAttributeDescriptions = nullptr;

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
                switch (info.colorBlend)
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
                    std::cerr << "color blend state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            // ビューポート, シザー
            VkPipelineViewportStateCreateInfo vpsci{};
            VkViewport viewport{};
            VkRect2D scissor{};

            {
                //上を反転するかどうかが謎
                if (info.viewport)
                {
                    viewport.x = info.viewport.value()[0][0];
                    viewport.y = info.viewport.value()[0][1];
                    viewport.minDepth = info.viewport.value()[0][2];

                    viewport.width = info.viewport.value()[1][0];
                    viewport.height = info.viewport.value()[1][1];
                    viewport.maxDepth = info.viewport.value()[1][2];
                }
                else
                {
                    const VkExtent3D& extent = rdsto.mExtent.value();
                    viewport.x = 0;
                    viewport.y = 0;
                    viewport.width = static_cast<float>(extent.width);
                    viewport.height = static_cast<float>(extent.height);
                    viewport.minDepth = 0;
                    viewport.maxDepth = extent.depth;
                }

                if (info.scissor)
                {
                    scissor.offset.x = static_cast<float>(info.scissor.value()[0][0]);
                    scissor.offset.y = static_cast<float>(info.scissor.value()[0][1]);
                    scissor.extent =
                    { static_cast<uint32_t>(info.scissor.value()[1][0]),
                    static_cast<uint32_t>(info.scissor.value()[1][1]) };
                }
                else
                {
                    scissor.offset = { 0, 0 };
                    scissor.extent =
                    { rdsto.mExtent.value().width,
                    rdsto.mExtent.value().height };
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
                switch (info.topology)
                {
                case Topology::eTriangleList:
                    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    break;
                case Topology::eTriangleStrip:
                    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                    break;
                default:
                    std::cerr << "primitive topology is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //ラスタライザ
            VkPipelineRasterizationStateCreateInfo rci{};
            {
                rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                switch (info.rasterizerState)
                {
                case RasterizerState::eDefault:
                    rci.polygonMode = VK_POLYGON_MODE_FILL;
                    rci.cullMode = VK_CULL_MODE_NONE;
                    rci.frontFace = VK_FRONT_FACE_CLOCKWISE;
                    rci.lineWidth = 1.f;
                    break;
                default:
                    std::cerr << "rasterizer state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //マルチサンプルステート
            VkPipelineMultisampleStateCreateInfo msci{};
            {
                msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                switch (info.multiSampleState)
                {
                case MultiSampleState::eDefault:
                    msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                    break;
                default:
                    std::cerr << "multisampling state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //デプスステンシルステート
            VkPipelineDepthStencilStateCreateInfo dsci{};
            {
                dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                switch (info.depthStencilState)
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
                    std::cerr << "depth stencil state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            std::array<VkPipelineShaderStageCreateInfo, 2> ssciArr{};

            result = createShaderModule(info.VS, VK_SHADER_STAGE_VERTEX_BIT, &ssciArr[0]);
            result = createShaderModule(info.FS, VK_SHADER_STAGE_FRAGMENT_BIT, &ssciArr[1]);

            {//DescriptorSetLayout構築

                VkDescriptorSetLayoutCreateInfo descLayoutci{};
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                rpo.layout = info.SRDesc.layout;

                for (int i = 0; i < info.SRDesc.layout.uniformBufferCount; ++i)
                {
                    bindings.emplace_back();
                    auto& b = bindings.back();

                    b.binding = i;
                    b.descriptorCount = 1;
                    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    b.stageFlags = VK_SHADER_STAGE_ALL;
                }

                for (int i = info.SRDesc.layout.uniformBufferCount; i < info.SRDesc.layout.uniformBufferCount + info.SRDesc.layout.combinedTextureCount; ++i)
                {
                    bindings.emplace_back();
                    auto& b = bindings.back();

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
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "failed to create descriptor set layout\n";
                        return result;
                    }

                    rpo.mDescriptorSetLayout = descriptorSetLayout;
                }
            }

            if (info.SRDesc.maxSetCount > 0)
            { //DescriptorPool構築
                std::array<VkDescriptorPoolSize, 2> sizes;
                sizes[0].descriptorCount = rpo.layout.uniformBufferCount;
                sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sizes[1].descriptorCount = rpo.layout.combinedTextureCount;
                sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

                const uint32_t sumDescriptor = rpo.layout.uniformBufferCount + rpo.layout.combinedTextureCount;

                VkDescriptorPoolCreateInfo dpci{};
                dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                if (info.SRDesc.maxSetCount < sumDescriptor)
                {
                    std::cerr << "invalid max SR count\n";
                    return Result::eFailure;
                }
                dpci.maxSets = info.SRDesc.maxSetCount;//*sumDescriptor; //エラーが出るようならこっち
                dpci.poolSizeCount = sizes.size();
                dpci.pPoolSizes = sizes.data();
                {
                    VkDescriptorPool descriptorPool;
                    result = checkVkResult(vkCreateDescriptorPool(mDevice, &dpci, nullptr, &descriptorPool));
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "failed to create Descriptor Pool\n";
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
                        std::cerr << "failed to create pipeline layout\n";
                        return result;
                    }

                    rpo.mPipelineLayout = pipelineLayout;
                }
            }

            {//繝代う繝励Λ繧､繝ｳ讒狗ｯ・
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
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "failed to create graphics pipeline\n";
                        return result;
                    }

                    rpo.mPipeline = pipeline;
                }
            }

            //ShaderModule破棄
            for (auto& ssci : ssciArr)
                vkDestroyShaderModule(mDevice, ssci.module, nullptr);
        }

        *pHandle = mNextRPHandle++;
        mRPMap.emplace(*pHandle, rpo);

        return Result::eSuccess;
    }

    Result Device::createCommandBuffer(const CommandList& commandList, HCommandBuffer* const pHandle)
    {
        Result result = Result::eSuccess;

        //コマンドオブジェクトの構築・初期化
        CommandObject co;

        {
            VkCommandBufferAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandPool = mCommandPool;
            ai.commandBufferCount = mMaxFrameNum;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            co.mCommandBuffers.resize(ai.commandBufferCount);
            result = checkVkResult(vkAllocateCommandBuffers(mDevice, &ai, co.mCommandBuffers.data()));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to allocate command buffers!\n";
                return result;
            }

            co.mFences.resize(mMaxFrameInFlight);

            VkFenceCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            for (size_t i = 0; i < co.mFences.size(); ++i)
            {
                result = checkVkResult(vkCreateFence(mDevice, &ci, nullptr, &co.mFences[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create fence!\n";
                    return result;
                }
            }

            std::cout << co.mFences.size() << "eawfa\n";

        }

        const InternalCommandList& cmdList = commandList.getInternalCommandData();

        //書き込み
        for (const auto& command : cmdList)
        {
            switch (command.first) //RTTI...
            {
            case CommandType::eBeginRenderPipeline:
                if (!std::holds_alternative<CmdBeginRenderPipeline>(command.second))
                    return Result::eFailure;
                result = cmdBeginRenderPipeline(co, std::get<CmdBeginRenderPipeline>(command.second));
                break;
            case CommandType::eEndRenderPipeline:
                if (!std::holds_alternative<CmdEndRenderPipeline>(command.second))
                    return Result::eFailure;
                result = cmdEndRenderPipeline(co, std::get<CmdEndRenderPipeline>(command.second));
                break;
            case CommandType::eBindVB:
                if (!std::holds_alternative<CmdBindVB>(command.second))
                    return Result::eFailure;
                result = cmdBindVB(co, std::get<CmdBindVB>(command.second));
                break;
            case CommandType::eBindIB:
                if (!std::holds_alternative<CmdBindIB>(command.second))
                    return Result::eFailure;
                result = cmdBindIB(co, std::get<CmdBindIB>(command.second));
                break;
            case CommandType::eBindSRSet:
                if (!std::holds_alternative<CmdBindSRSet>(command.second))
                    return Result::eFailure;
                result = cmdBindSRSet(co, std::get<CmdBindSRSet>(command.second));
                break;
            case CommandType::eRender:
                if (!std::holds_alternative<CmdRender>(command.second))
                    return Result::eFailure;
                result = cmdRender(co, std::get<CmdRender>(command.second));
                break;
            default:
                std::cerr << "invalid command!\n";
                result = Result::eFailure;
                break;
            }
        }

        *pHandle = mNextCBHandle++;
        mCommandBufferMap.emplace(*pHandle, co);

        return result;
    }

    Result Device::execute(const HCommandBuffer& handle, const HSwapchain& swapchain)
    {
        Result result = Result::eSuccess;

        if (mCommandBufferMap.count(handle) <= 0)
        {
            std::cerr << "invalid commandbuffer handle!\n";
            return Result::eFailure;
        }

        if (mSwapchainMap.count(handle) <= 0)
        {
            std::cerr << "invalid swapchain handle!\n";
            return Result::eFailure;
        }

        auto& so = mSwapchainMap[handle];
        auto& co = mCommandBufferMap[handle];
        result = checkVkResult(vkWaitForFences(mDevice, 1, &co.mFences[mCurrentFrame], VK_TRUE, UINT64_MAX));
        if (result != Result::eSuccess)
        {
            std::cerr << "Failed to wait fence!\n";
            return result;
        }

        uint32_t swapchainImageIndex = 0;

        result = checkVkResult(
            vkAcquireNextImageKHR(
                mDevice,
                so.mSwapchain.value(),
                UINT64_MAX,
                mPresentCompletedSems[mCurrentFrame],
                VK_NULL_HANDLE,
                &swapchainImageIndex));

        std::cout << swapchainImageIndex << " : swapchain image index\n";

        if (result != Result::eSuccess)
        {
            std::cerr << "failed to acquire next swapchain image!\n";
            return result;
        }

        if (imagesInFlight[swapchainImageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(mDevice, 1, &imagesInFlight[swapchainImageIndex], VK_TRUE, UINT64_MAX);
        imagesInFlight[swapchainImageIndex] = co.mFences[mCurrentFrame];


        // コマンドを実行（送信)
        VkSubmitInfo submitInfo{};
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &(co.mCommandBuffers[swapchainImageIndex]);
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &mPresentCompletedSems[mCurrentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &mRenderCompletedSems[mCurrentFrame];
        result = checkVkResult(vkResetFences(mDevice, 1, &co.mFences[mCurrentFrame]));
        if (result != Result::eSuccess)
        {
            std::cerr << "failed to reset fence!\n";
            return result;
        }

        result = checkVkResult(vkQueueSubmit(mDeviceQueue, 1, &submitInfo, co.mFences[mCurrentFrame]));
        if (result != Result::eSuccess)
        {
            std::cerr << "failed to submit cmd to queue!\n";
            return result;
        }

        //present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &so.mSwapchain.value();
        presentInfo.pImageIndices = &swapchainImageIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderCompletedSems[mCurrentFrame];

        result = checkVkResult(vkQueuePresentKHR(mDeviceQueue, &presentInfo));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to present queue!\n";
            return result;
        }

        //vkQueueWaitIdle(mDeviceQueue);

        mCurrentFrame = (mCurrentFrame + 1) % mMaxFrameInFlight;

        //
        // {
        //     if (mRCState->mDescriptorSet && mRCState->mDescriptorPool)
        //         vkFreeDescriptorSets(mDevice, mRCState->mDescriptorPool.value(), 1, &mRCState->mDescriptorSet.value());
        //     if (mRCState->mDescriptorPool)
        //         vkDestroyDescriptorPool(mDevice, mRCState->mDescriptorPool.value(), nullptr);
        //     mRCState = std::nullopt; //解放
        //     RunningCommandState tmp;
        //     mRCState = tmp; //新しいインスタンス取得
        // }

        return Result::eSuccess;
    }

    // Result Device::present(const HSwapchain &handle)
    // {
    //     Result result;

    //     if (mSwapchainMap.count(handle) <= 0)
    //     {
    //         std::cerr << "invalid swapchain handle!\n";
    //         return Result::eFailure;
    //     }

    //     auto& so = mSwapchainMap[handle];

    //     VkPresentInfoKHR presentInfo{};
    //     presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //     presentInfo.swapchainCount = 1;
    //     presentInfo.pSwapchains = &so.mSwapchain.value();
    //     presentInfo.pImageIndices = &mFrameIndex;
    //     presentInfo.waitSemaphoreCount = 1;
    //     presentInfo.pWaitSemaphores = &mRenderCompletedSem;

    //     result = checkVkResult(vkQueuePresentKHR(mDeviceQueue, &presentInfo));
    //     if (Result::eSuccess != result)
    //     {
    //         std::cerr << "Failed to present queue!\n";
    //         return result;
    //     }

    //     vkQueueWaitIdle(mDeviceQueue);

    //     mFrameIndex = (mFrameIndex + 1) % mMaxFrameNum;

    //     return Result::eSuccess;
    // }

    Result Device::cmdBeginRenderPipeline(CommandObject& co, const CmdBeginRenderPipeline& info)
    {
        Result result = Result::eSuccess;

        auto& rpo = mRPMap[info.RPHandle];
        auto& rdsto = mRDSTMap[rpo.mHRenderDST];
        VkRenderPassBeginInfo bi{};

        VkCommandBufferBeginInfo commandBI{};
        commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBI.flags = 0;
        commandBI.pInheritanceInfo = nullptr;

        if (rdsto.mHSwapchain)
        {
            auto& swapchain = mSwapchainMap[rdsto.mHSwapchain.value()];

            //VkClearValue clearValues[2];
            //clearValues[0] = { 1.0, 0, 0, 0 };
            //clearValues[1] = { 0, 0 };

            VkClearValue clearValue = { 0, 0, 0, 0 };

            bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            bi.renderPass = rdsto.mRenderPass.value();
            bi.renderArea.offset = { 0, 0 };
            bi.renderArea.extent = swapchain.mSwapchainExtent;

            // クリア値
            if (rdsto.mDepthTestEnable)
            {
                std::cerr << "this process does not implemented yet!";
                exit(-1);
                //bi.clearValueCount = 2;
                //bi.pClearValues = clearValues;
            }
            else
            {
                bi.clearValueCount = 1;
                bi.pClearValues = &clearValue;
            }
        }
        else
        {
            //TODO: テクスチャレンダリング時のここ
            std::cerr << "this process does not implemented yet!\n";
        }


        for (size_t i = 0; i < co.mCommandBuffers.size(); ++i)
        {
            auto& command = co.mCommandBuffers[i];
            result = checkVkResult(vkBeginCommandBuffer(command, &commandBI));
            if (result != Result::eSuccess)
            {
                std::cerr << "Failed to begin command buffer!\n";
                return result;
            }

            bi.framebuffer = rdsto.mFramebuffers[i].value();

            vkCmdBeginRenderPass(command, &bi, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, rpo.mPipeline.value());
        }

        return Result::eSuccess;
    }

    Result Device::cmdEndRenderPipeline(CommandObject& co, const CmdEndRenderPipeline& info)
    {
        Result result;
        // コマンド・レンダーパス終了
        for (const auto& command : co.mCommandBuffers)
        {
            vkCmdEndRenderPass(command);
            result = checkVkResult(vkEndCommandBuffer(command));
            if (result != Result::eSuccess)
            {
                std::cerr << "Failed to end command buffer!\n";
                return result;
            }
        }

        return Result::eSuccess;
    }

    Result Device::cmdBindVB(CommandObject& co, const CmdBindVB& info)
    {
        if (mBufferMap.count(info.VBHandle) <= 0)
            return Result::eFailure;
        auto& vbo = mBufferMap[info.VBHandle];

        for (const auto& command : co.mCommandBuffers)
            vkCmdBindVertexBuffers(command, 0, 1, &vbo.mBuffer.value(), nullptr);

        return Result::eSuccess;
    }

    Result Device::cmdBindIB(CommandObject& co, const CmdBindIB& info)
    {
        if (mBufferMap.count(info.IBHandle) <= 0)
            return Result::eFailure;
        auto& ibo = mBufferMap[info.IBHandle];

        for (const auto& command : co.mCommandBuffers)
            vkCmdBindIndexBuffer(command, ibo.mBuffer.value(), 0, VK_INDEX_TYPE_UINT32);

        return Result::eSuccess;
    }

    Result Device::cmdBindSRSet(CommandObject& co, const CmdBindSRSet& info)
    {
        Result result = Result::eSuccess;

        if (!co.mHRPO)
        {
            std::cerr << "render pipeline object is not registed yet!\n";
            return Result::eFailure;
        }

        auto& rpo = mRPMap[co.mHRPO.value()];

        if (!rpo.mDescriptorPool)
        {
            std::cerr << "descriptor pool is nothing!\n";
            return Result::eFailure;
        }

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
                std::cerr << "failed to allocate descriptor set\n";
                return result;
            }

            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.reserve(rpo.layout.combinedTextureCount + rpo.layout.uniformBufferCount);

            uint32_t count = 0;
            for (auto& hub : info.SRSet.uniformBuffer)
            {
                VkDescriptorBufferInfo dbi;

                if (mBufferMap.count(hub) <= 0)
                {
                    std::cerr << "invalid uniform buffer handle!\n";
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
            for (auto& hct : info.SRSet.combinedTexture)
            {
                VkDescriptorImageInfo dii;

                if (mImageMap.count(hct) <= 0)
                {
                    std::cerr << "invalid combined texture handle!\n";
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

        co.mDescriptorSets.emplace_back(descriptorSet);

        for (const auto& command : co.mCommandBuffers)
            vkCmdBindDescriptorSets(
                command,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                rpo.mPipelineLayout.value(),
                0,
                1,
                &descriptorSet,
                0,
                nullptr);

        return Result::eSuccess;
    }

    Result Device::cmdRenderIndexed(CommandObject& co, const CmdRenderIndexed& info)
    {
        for (const auto& command : co.mCommandBuffers)
            vkCmdDrawIndexed
            (
                command,
                info.indexCount,
                info.instanceCount,
                info.firstIndex,
                info.vertexOffset,
                info.firstInstance
            );

        return Result::eSuccess;
    }

    Result Device::cmdRender(CommandObject& co, const CmdRender& info)
    {

        for (const auto& command : co.mCommandBuffers)
            vkCmdDraw
            (
                command,
                info.vertexCount,
                info.instanceCount,
                info.vertexOffset,
                info.firstInstance
            );

        return Result::eSuccess;
    }

    // Result Device::writeCommand(const Command& command)//ボトルネック
    // {
    //     mWroteCommands.emplace_back(command);

    //     switch (command.type) //RTTIｪ...
    //     {
    //     case CommandType::eCmdBeginRenderPipeline:
    //         if (!std::holds_alternative<CmdBeginRenderPipeline>(command.info))
    //             return Result::eFailure;

    //         std::get<CmdBeginRenderPipeline>(command.info);

    //         if (mRPMap.count(std::get<CmdBeginRenderPipeline>(command.info).RPHandle) <= 0)
    //         {
    //             std::cerr << "invalid RP handle!";
    //             return Result::eFailure;
    //         }

    //         mRCState->mHRPO = std::get<CmdBeginRenderPipeline>(command.info).RPHandle;
    //         break;

    //     case CommandType::eCmdSetShaderResource:
    //         if (!std::holds_alternative<CmdSetShaderResource>(command.info))
    //             return Result::eFailure;

    //         mRCState->maxSR +=
    //             std::get<CmdSetShaderResource>(command.info).shaderResource.uniformBuffer.size() +
    //             std::get<CmdSetShaderResource>(command.info).shaderResource.combinedTexture.size();
    //         break;

    //     case CommandType::eCmdEndRenderPipeline:
    //         break;
    //     case CommandType::eCmdSetVB:
    //         break;
    //     case CommandType::eCmdSetIB:
    //         break;
    //     case CommandType::eCmdRender:
    //         break;
    //     default:
    //         std::cerr << "invalid command!\n";
    //         return Result::eFailure;
    //         break;
    //     }

    //     return Result::eSuccess;
    // }
};