#include "../include/Context.hpp"

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
            std::cerr << "[" << pLayerPrefix << "] ";
        }
        std::cerr << pMessage << std::endl;

        return ret;
    }

    Context::~Context()
    {
        if(mIsInitialized)
        {
            std::cerr << "You forgot destroy context explicitly!\n";
            destroy();
        }
    }

    Result Context::initialize(const InitializeInfo &initializeInfo)
    {
        mInitializeInfo = initializeInfo;
        Result result;
        mMaxFrameNum = initializeInfo.frameCount; //temporary set for safety
        mCurrentFrame = 0;

        std::cerr << "initialize started...\n";

        //GLFW initialization

        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW!\n";
            exit(-1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        std::cerr << "GLFW initialized\n";

        //instance
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

        //select physical device
        result = selectPhysicalDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "selected Physical Device\n";

        //search graphics queue
        result = searchGraphicsQueueIndex();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "found index of GraphicsQueue\n";

        //logical device
        result = createDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkDevice\n";

        //command pool
        result = createCommandPool();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkCommandPool\n";

        std::cerr << "all initialize processes succeeded\n";
        mIsInitialized = true;

        return Result::eSuccess;
    }

    Result Context::destroy()
    {
        if(!mIsInitialized)
        {
            std::cerr << "this context was not initialized!\n";
            return Result::eFailure;
        }

        std::cerr << "destroying...\n";

        if (VK_SUCCESS != vkDeviceWaitIdle(mDevice))
            std::cerr << "Failed to wait device idol\n";

        for (auto &e : mBufferMap)
        {
            if (e.second.mBuffer)
                vkDestroyBuffer(mDevice, e.second.mBuffer.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);
        }
        mBufferMap.clear();
        std::cerr << "destroyed user allocated buffers\n";

        for (auto &e : mImageMap)
        {
            if (e.second.mView)
                vkDestroyImageView(mDevice, e.second.mView.value(), nullptr);

            if (e.second.usage == TextureUsage::eSwapchainImage) //avoid destroying SwapchainImage
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

        for (auto &e : mRDSTMap)
        {
            for (auto &f : e.second.mFramebuffers)
                vkDestroyFramebuffer(mDevice, f.value(), nullptr);

            if (e.second.mRenderPass)
                vkDestroyRenderPass(mDevice, e.second.mRenderPass.value(), nullptr);
        }

        for (auto &e : mRPMap)
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

        for (auto &e : mCommandBufferMap)
        {
            vkFreeCommandBuffers(mDevice, mCommandPool, uint32_t(e.second.mCommandBuffers.size()), e.second.mCommandBuffers.data());
            //e.second.mCommandBuffers.clear();
            for (auto &f : e.second.mFences)
                vkDestroyFence(mDevice, f, nullptr);
            for (const auto &pcSem : e.second.mPresentCompletedSems)
                vkDestroySemaphore(mDevice, pcSem, nullptr);
            for (const auto &rcSem : e.second.mRenderCompletedSems)
                vkDestroySemaphore(mDevice, rcSem, nullptr);
            std::cerr << "destroyed semaphores\n";
        }

        std::cerr << "destroyed fences\n";
        std::cerr << "destroyed command buffers\n";
        //for(auto& e : mSamplerMap)
        //{
        //    vkDestroySampler(mDevice, e.second, nullptr);
        //}
        //mSamplerMap.clear();



        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        std::cerr << "destroyed command pool\n";

        for (auto &so : mWindowMap)
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

        mWindowMap.clear();
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
        mIsInitialized = false;

        return Result::eSuccess;
    }

    Result Context::checkVkResult(VkResult result)
    {
        if (result != VK_SUCCESS)
        {
            std::cerr << "ERROR!\nvulkan error : " << result << "\n";
            return Result::eFailure; //TODO: switch and error handling
        }
        return Result::eSuccess;
    }

    Result Context::createInstance()
    {
        Result result;

        std::vector<const char *> extensions;
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = mInitializeInfo.appName.c_str();
        appInfo.pEngineName = ENGINE_NAME;
        appInfo.apiVersion = VK_API_VERSION_1_1;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        //get extention properties
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

            for (const auto &v : props)
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
            // debug layer, depending on SDK version
            const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
            //const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
            ci.enabledLayerCount = 1;
            ci.ppEnabledLayerNames = layers;
        }
        else
        {
            ci.enabledLayerCount = 0;
            ci.ppEnabledLayerNames = nullptr;
        }

        result = checkVkResult(vkCreateInstance(&ci, nullptr, &mInstance));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to enable debug layer! creating VkInstance without debug layer...\n";
            ci.enabledLayerCount = 0;
            ci.ppEnabledLayerNames = nullptr;
            result = checkVkResult(vkCreateInstance(&ci, nullptr, &mInstance));
        }

        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to create Vulkan instance!\n";
            return result;
        }

        return Result::eSuccess;
    }

    Result Context::selectPhysicalDevice()
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

        //select suitable device
        for (const auto &pd : physDevs)
        {
            std::optional<uint32_t> indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto &queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices = i;
                
                if (indices)
                {
                    mPhysDev = pd;
                    break;
                }
    
                i++;
            }
        }

        if (mPhysDev == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        //get physical memory properties
        vkGetPhysicalDeviceMemoryProperties(mPhysDev, &mPhysMemProps);

        return Result::eSuccess;
    }

    Result Context::searchGraphicsQueueIndex()
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

    Result Context::createDevice()
    {
        Result result;

        std::vector<VkExtensionProperties> devExtProps;

        {
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

        vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mDeviceQueue);

        return Result::eSuccess;
    }

    Result Context::createCommandPool()
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

    Result Context::createSurface(WindowObject &wo)
    {
        Result result;

        VkSurfaceKHR surface;
        result = checkVkResult(glfwCreateWindowSurface(mInstance, wo.mpWindow.value(), nullptr, &surface));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to create window surface!\n";
            return result;
        }
        wo.mSurface = surface;

        result = selectSurfaceFormat(wo, VK_FORMAT_B8G8R8A8_UNORM);
        if (Result::eSuccess != result)
        {
            std::cout << "Failed to select surface format!\n";
            return result;
        }

        result = checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysDev, wo.mSurface.value(), &wo.mSurfaceCaps));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to get physical device surface capability!\n";
            return result;
        }

        VkBool32 isSupport;
        result = checkVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysDev, mGraphicsQueueIndex, wo.mSurface.value(), &isSupport));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Context::selectSurfaceFormat(WindowObject &wo, VkFormat format)
    {
        Result result;

        uint32_t surfaceFormatCount = 0; //get count of formats
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, wo.mSurface.value(), &surfaceFormatCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount); //get actual format
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysDev, wo.mSurface.value(), &surfaceFormatCount, formats.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // search matched format
        for (const auto &f : formats)
        {
            if (f.format == format)
            {
                wo.mSurfaceFormat = f;
            }
        }

        return Result::eSuccess;
    }

    Result Context::createSwapchain(WindowObject &wo)
    {
        Result result;

        mMaxFrameNum = std::max(wo.mSurfaceCaps.minImageCount, mMaxFrameNum);
        mMaxFrameInFlight = mMaxFrameNum - 1;

        auto &extent = wo.mSurfaceCaps.currentExtent;
        if (extent.width <= 0u || extent.height <= 0u)
        {
            // ignore invalid param and use window size
            int width, height;
            glfwGetWindowSize(wo.mpWindow.value(), &width, &height);
            extent.width = static_cast<uint32_t>(width);
            extent.height = static_cast<uint32_t>(height);
        }

        wo.mPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t queueFamilyIndices[] = {mGraphicsQueueIndex};
        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = wo.mSurface.value();
        ci.minImageCount = mMaxFrameNum;
        ci.imageFormat = wo.mSurfaceFormat.format;
        ci.imageColorSpace = wo.mSurfaceFormat.colorSpace;
        ci.imageExtent = extent;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.preTransform = wo.mSurfaceCaps.currentTransform;
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

            wo.mSwapchain = swapchain;
        }

        wo.mSwapchainExtent = extent;

        return Result::eSuccess;
    }

    Result Context::createSwapchainImages(WindowObject &wo)
    {
        Result result;

        uint32_t imageCount;
        result = checkVkResult(vkGetSwapchainImagesKHR(mDevice, wo.mSwapchain.value(), &imageCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkImage> swapchainImages(imageCount);
        result = checkVkResult(vkGetSwapchainImagesKHR(mDevice, wo.mSwapchain.value(), &imageCount, swapchainImages.data()));
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
            ci.format = wo.mSurfaceFormat.format;
            ci.components =
                {
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                };
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
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
            io.extent = {wo.mSwapchainExtent.width, wo.mSwapchainExtent.height, static_cast<uint32_t>(1)};
            io.format = wo.mSurfaceFormat.format;

            wo.mHSwapchainImages.emplace_back(mNextTextureHandle++);
            mImageMap.emplace(wo.mHSwapchainImages.back(), io);
        }

        return Result::eSuccess;
    }

    // Result Context::createSemaphores()
    // {
    //     Result result = Result::eSuccess;

    //     VkSemaphoreCreateInfo ci{};
    //     ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    //     mPresentCompletedSems.resize(mMaxFrameInFlight);
    //     mRenderCompletedSems.resize(mMaxFrameInFlight);

    //     for (size_t i = 0; i < mMaxFrameInFlight; ++i)
    //     {
    //         result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mRenderCompletedSems[i]));
    //         if (Result::eSuccess != result)
    //         {
    //             std::cerr << "Failed to create render completed semaphore!\n";
    //             return result;
    //         }
    //     }

    //     for (size_t i = 0; i < mMaxFrameInFlight; ++i)
    //     {
    //         result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &mPresentCompletedSems[i]));
    //         if (Result::eSuccess != result)
    //         {
    //             std::cerr << "Failed to create present completed semaphore!\n";
    //             return result;
    //         }
    //     }

    //     imagesInFlight.resize(mMaxFrameNum, VK_NULL_HANDLE);

    //     return Result::eSuccess;
    // }

    // Result Context::getSwapchainImages(HSwapchain handle, std::vector<HTexture>& handlesRef)
    // {
    //     if(mWindowMap.count(handle) <= 0)
    //     {
    //         return Result::eFailure;
    //     }

    //     auto &images = mWindowMap[handle].mHSwapchainImages;;
    //     handlesRef.resize(images.size());
    //     for(size_t i = 0; i < handlesRef.size(); ++i)
    //     {
    //         handlesRef.at(i) = images[i];
    //     }

    //     return Result::eSuccess;
    // }

    // Result Context::getSwapchainDepthBuffer(HSwapchain handle, HTexture *pHandle)
    // {
    //     if(mWindowMap.count(handle) <= 0)
    //     {
    //         return Result::eFailure;
    //     }

    //     *pHandle = mWindowMap[handle].mHDepthBuffer;

    //     return Result::eSuccess;
    // }

    Result Context::enableDebugReport()
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

    Result Context::disableDebugReport()
    {

        if (mvkDestroyDebugReportCallbackEXT)
        {
            mvkDestroyDebugReportCallbackEXT(mInstance, mDebugReport, nullptr);
        }

        std::cerr << "disabled debug mode\n";

        return Result::eSuccess;
    }

    uint32_t Context::getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
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

    Result Context::createWindow(const WindowInfo &info, HWindow &handle_out)
    {
        Result result = Result::eSuccess;
        WindowObject wo;

        wo.mpWindow = std::make_optional(glfwCreateWindow(
            info.width,
            info.height,
            info.windowName.c_str(),
            nullptr,
            nullptr));

        if (!wo.mpWindow.value())
        {
            std::cerr << "Failed to create GLFW Window!\n";
            destroy();
            exit(-1);
        }

        result = createSurface(wo);
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkSurfaceKHR\n";

        result = createSwapchain(wo);
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkSwapChainKHR\n";

        result = createSwapchainImages(wo);
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created swapchain images\n";

        result = createDepthBuffer(wo);
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created swapchain depthbuffer\n";

        //set swapchain object
        handle_out = mNextWindowHandle++;
        mWindowMap.emplace(handle_out, wo);

        return result;
    }

    Result Context::createBuffer(const BufferInfo &info, HBuffer& handle_out)
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
            ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, fb);

            // allocate device memory
            {
                VkDeviceMemory memory;
                result = checkVkResult(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                bo.mMemory = memory;
            }

            // bind memory
            result = checkVkResult(vkBindBufferMemory(mDevice, bo.mBuffer.value(), bo.mMemory.value(), 0));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        handle_out = mNextBufferHandle++;
        mBufferMap.emplace(handle_out, bo);

        return Result::eSuccess;
    }

    Result Context::writeBuffer(const size_t size, const void *const pData, const HBuffer &handle)
    {
        Result result = Result::eFailure;
        if (mBufferMap.count(handle) <= 0)
            return result;

        BufferObject &bo = mBufferMap[handle];

        void *p; //mapping dst address

        result = checkVkResult(vkMapMemory(mDevice, bo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
        if (result != Result::eSuccess)
            return result;
        memcpy(p, pData, size);
        vkUnmapMemory(mDevice, bo.mMemory.value());

        return Result::eSuccess;
    }

    Result Context::createTexture(const TextureInfo &info, HTexture &handle_out)
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
                    io.mSizeOfChannel = 3;
                    break;
                case TextureFormatType::eFloat:
                    io.format = VK_FORMAT_R16G16B16_SFLOAT;
                    io.mSizeOfChannel = 6;
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
                    io.mSizeOfChannel = 4;
                    break;
                case TextureFormatType::eFloat:
                    io.format = VK_FORMAT_R16G16B16A16_SFLOAT;
                    io.mSizeOfChannel = 8;
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
                ci.extent = {uint32_t(info.width), uint32_t(info.height), 1};
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
                // case TextureUsage::eUnordered: 
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

        // calc memory size
        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(mDevice, io.mImage.value(), &reqs);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        // decide memory type
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
        // allocate memory
        {
            VkDeviceMemory memory;
            vkAllocateMemory(mDevice, &ai, nullptr, &memory);

            io.mMemory = memory;
        }
        // bind memory
        vkBindImageMemory(mDevice, io.mImage.value(), io.mMemory.value(), 0);

        {
            //view
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
                    VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
                break;
            default:
                ci.subresourceRange = {
                    VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
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
            //TODO: impl changing sampler
            VkSamplerCreateInfo sci{}; 
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

        handle_out = mNextTextureHandle++;
        mImageMap.emplace(handle_out, io);

        return Result::eSuccess;
    }

    Result Context::createTextureFromFile(const char *fileName, HTexture &handle_out)
    {
        ImageObject io;
        Result result;

        stbi_uc *pImage = nullptr;
        int width = 0, height = 0, channel = 0;
        io.format = VK_FORMAT_R8G8B8A8_SRGB; //fixed format
        io.mSizeOfChannel = 4;
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

            io.extent = {uint32_t(width), uint32_t(height), 1};

            // image
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.extent = io.extent;
            ci.format = io.format;
            ci.imageType = VK_IMAGE_TYPE_2D;
            ci.arrayLayers = 1;
            ci.mipLevels = 1;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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

        // calc memory size
        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(mDevice, io.mImage.value(), &reqs);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        // decide memory type
        VkMemoryPropertyFlagBits fb;
        fb = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        io.mIsHostVisible = true;
        ai.memoryTypeIndex = getMemoryTypeIndex(reqs.memoryTypeBits, fb);

        // allocate device memory
        {
            VkDeviceMemory memory;
            vkAllocateMemory(mDevice, &ai, nullptr, &memory);

            io.mMemory = memory;
        }
        // bind device memory
        vkBindImageMemory(mDevice, io.mImage.value(), io.mMemory.value(), 0);

        {
            //view
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.image = io.mImage.value();
            ci.format = io.format;

            ci.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

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
            VkSamplerCreateInfo sci{};
            sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.pNext = nullptr;
            sci.minFilter = VK_FILTER_LINEAR;
            sci.magFilter = VK_FILTER_LINEAR;
            sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sci.anisotropyEnable = VK_FALSE;
            sci.maxAnisotropy = 16.f;
            sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            sci.compareOp = VK_COMPARE_OP_ALWAYS;
            sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VkSampler sampler;
            result = checkVkResult(vkCreateSampler(mDevice, &sci, nullptr, &sampler));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to create VkSampler!\n";
                return result;
            }

            io.mSampler = sampler;
        }

        handle_out = mNextTextureHandle++;
        mImageMap.emplace(handle_out, io);

        writeTexture(pImage, handle_out);

        if (pImage != nullptr)
            stbi_image_free(pImage);

        return Result::eSuccess;
    }

    Result Context::writeTexture(const void *const pData, const HTexture &handle)
    {
        Result result = Result::eFailure;

        if (mImageMap.count(handle) <= 0)
            return Result::eFailure;

        ImageObject &io = mImageMap[handle];

        BufferObject stagingBo;
        {
            size_t imageSize = io.extent.width * io.extent.height * io.extent.depth * io.mSizeOfChannel;
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

                {
                    VkDeviceMemory memory;
                    vkAllocateMemory(mDevice, &ai, nullptr, &memory);
                    stagingBo.mMemory = memory;
                }

                vkBindBufferMemory(mDevice, stagingBo.mBuffer.value(), stagingBo.mMemory.value(), 0);
            }

            void *p;
            result = checkVkResult(vkMapMemory(mDevice, stagingBo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
            if (Result::eSuccess != result)
                return result;

            memcpy(p, pData, imageSize);
            vkUnmapMemory(mDevice, stagingBo.mMemory.value());
        }

        VkBufferImageCopy copyRegion{};
        copyRegion.imageExtent =
            {
                static_cast<uint32_t>(io.extent.width),
                static_cast<uint32_t>(io.extent.height),
                static_cast<uint32_t>(io.extent.depth)};
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

        //end copying
        vkDeviceWaitIdle(mDevice);
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &command);

        //release staging buffer
        vkFreeMemory(mDevice, stagingBo.mMemory.value(), nullptr);
        vkDestroyBuffer(mDevice, stagingBo.mBuffer.value(), nullptr);

        return Result::eSuccess;
    }

    Result Context::setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier imb{};
        imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.oldLayout = oldLayout;
        imb.newLayout = newLayout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imb.image = image;

        //final stage that write to resource in pipelines
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        //next state that write to resource in pipelines
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

    Result Context::createDepthBuffer(WindowObject &wo)
    {
        Result result;
        ImageObject io;

        {
            VkImageCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.imageType = VK_IMAGE_TYPE_2D;
            ci.format = VK_FORMAT_D32_SFLOAT;
            ci.extent.width = wo.mSwapchainExtent.width;
            ci.extent.height = wo.mSwapchainExtent.height;
            ci.extent.depth = 1;
            ci.mipLevels = 1;
            ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            ci.samples = VK_SAMPLE_COUNT_1_BIT;
            ci.arrayLayers = 1;

            
            io.format = ci.format;
            io.extent = ci.extent;
            io.mSizeOfChannel = 4;
            
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
                    VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

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
        wo.mHDepthBuffer = mNextTextureHandle++;

        return Result::eSuccess;
    }

    Result Context::createShaderModule(const Shader &shader, const VkShaderStageFlagBits &stage, VkPipelineShaderStageCreateInfo *pSSCI)
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
        ci.pCode = reinterpret_cast<uint32_t *>(filedata.data());
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

    Result Context::createRenderDST(const HWindow &handle, bool depthTestEnable, HRenderDST &handle_out)
    {
        Result result;
        RenderDSTObject rdsto;

        rdsto.mDepthTestEnable = depthTestEnable;

        //Renderpass, framebuffer

        if (mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid swapchain handle\n";
            return Result::eFailure;
        }

        rdsto.mHWindow = handle;
        auto &swapchain = mWindowMap[handle];

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

            adVec.reserve(2);

            const auto &rdst = swapchain.mHSwapchainImages[0]; //only for info

            if (mImageMap.count(rdst) <= 0)
            {
                std::cerr << "invalid swapchain image handle\n";
                return Result::eFailure;
            }

            auto &io = mImageMap[rdst];
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

            //if use depthBuffer, create attachment
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
            const auto &depth = mImageMap[swapchain.mHDepthBuffer];
            fbci.attachmentCount = 2;
            for (auto &h : swapchain.mHSwapchainImages)
            {
                const auto &img = mImageMap[h];
                std::array<VkImageView, 2> ivArr = {img.mView.value(), depth.mView.value()};
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
            for (auto &h : swapchain.mHSwapchainImages)
            {
                fbci.pAttachments = &mImageMap[h].mView.value();

                VkFramebuffer frameBuffer;
                result = checkVkResult(vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to create framebuffer!\n";
                    return result;
                }

                rdsto.mFramebuffers.emplace_back(frameBuffer);
            }
        }

        handle_out = mNextRenderDSTHandle++;
        mRDSTMap.emplace(handle_out, rdsto);

        return Result::eSuccess;
    }

    Result Context::createRenderDST(const std::vector<HTexture> &textures, HRenderDST &handle_out)
    {
        Result result;
        RenderDSTObject rdsto;

        rdsto.mHWindow = std::nullopt;

        VkRenderPassCreateInfo ci{};
        std::vector<VkAttachmentDescription> adVec;
        std::vector<VkAttachmentReference> arVec;
        std::optional<HTexture> hDepthBuffer;

        if (mInitializeInfo.debugFlag) //for debug mode
        {
            for (auto &tex : textures)
            {
                if (mImageMap.count(tex) <= 0)
                {
                    std::cerr << "invalid texture handle\n";
                    return Result::eFailure;
                }
                auto &io = mImageMap[tex];

                //usage check
                if (io.usage != TextureUsage::eColorTarget || io.usage != TextureUsage::eDepthStencilTarget)
                {
                    std::cerr << "invalid texture usage\n";
                    return Result::eFailure;
                }

                //extent substitute and check
                if (!rdsto.mExtent)
                    rdsto.mExtent = io.extent;
                if (rdsto.mExtent.value().width != io.extent.width || rdsto.mExtent.value().height != io.extent.height || rdsto.mExtent.value().depth != io.extent.depth)
                {
                    std::cerr << "invalid texture extent\n";
                    return Result::eFailure;
                }
            }
        }

        //Renderpass, Framebuffer
        for (auto &tex : textures)
        {

            auto &io = mImageMap[tex];
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
                hDepthBuffer = tex;
                adVec.pop_back();
                arVec.pop_back();
                break;
            default: //did not expected
                return Result::eFailure;
                break;
            }
        }

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount = arVec.size();
        subpassDesc.pColorAttachments = arVec.data();

        //if use depthBuffer, create attachment
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
        if (Result::eSuccess != result)
        {
            return result;
        }

        handle_out = mNextRenderDSTHandle++;
        mRDSTMap.emplace(handle_out, rdsto);

        return Result::eSuccess;
    }

    Result Context::createRenderPipeline(const RenderPipelineInfo &info, HRenderPipeline& handle_out)
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

        //pWO
        {
            
            VkVertexInputBindingDescription ib;
            
            std::vector<VkVertexInputAttributeDescription> ia_vec;
            
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
                        std::cerr << "invalid size!\n";
                        return Result::eFailure;
                    }

                    size_t offset = 0;
                    for (size_t i = 0; i < info.vertexLayout.value().layouts.size(); ++i)
                    {
                        ia_vec.emplace_back();
                        ia_vec.back().binding = 0;
                        ia_vec.back().location = i;
                        ia_vec.back().offset = offset;
                        switch (info.vertexLayout.value().layouts[i].first)
                        {
                        case ResourceType::eInt32:
                            ia_vec.back().format = VK_FORMAT_R32_SINT;
                            offset += 4;
                            break;
                        case ResourceType::eUint32:
                            ia_vec.back().format = VK_FORMAT_R32_UINT;
                            offset += 4;
                            break;
                        case ResourceType::eFVec2:
                            ia_vec.back().format = VK_FORMAT_R32G32_SFLOAT;
                            offset += 8;
                            break;
                        case ResourceType::eFVec3:
                            ia_vec.back().format = VK_FORMAT_R32G32B32_SFLOAT;
                            offset += 12;
                            break;
                        case ResourceType::eFVec4:
                            ia_vec.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;
                            offset += 16;
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
            else //vertex input state
            {

                visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                visci.pNext = nullptr;
                visci.vertexBindingDescriptionCount = 0;
                visci.pVertexBindingDescriptions = nullptr;
                visci.vertexAttributeDescriptionCount = 0;
                visci.pVertexAttributeDescriptions = nullptr;
            }

            //color blend
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

            //viewport and scissor
            VkPipelineViewportStateCreateInfo vpsci{};
            VkViewport viewport{};
            VkRect2D scissor{};

            {
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
                    const VkExtent3D &extent = rdsto.mExtent.value();
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

            //input assembly
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

            //rasterization
            VkPipelineRasterizationStateCreateInfo rci{};
            {
                rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                switch (info.rasterizerState)
                {
                case RasterizerState::eDefault:
                    rci.polygonMode = VK_POLYGON_MODE_FILL;
                    rci.cullMode = VK_CULL_MODE_BACK_BIT;
                    rci.frontFace = VK_FRONT_FACE_CLOCKWISE;
                    rci.lineWidth = 1.f;
                    break;
                default:
                    std::cerr << "rasterizer state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            //multi sampling
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

            //depth stencil
            VkPipelineDepthStencilStateCreateInfo dsci{};
            {
                dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                switch (info.depthStencilState)
                {
                case DepthStencilState::eNone:
                    dsci.depthTestEnable = dsci.stencilTestEnable = VK_FALSE;
                    dsci.depthWriteEnable = VK_FALSE;
                    break;
                case DepthStencilState::eDepth:
                    dsci.depthTestEnable = VK_TRUE;
                    dsci.depthWriteEnable = VK_TRUE;
                    dsci.stencilTestEnable = VK_FALSE;
                    dsci.depthCompareOp = VK_COMPARE_OP_LESS;
                    dsci.depthBoundsTestEnable = VK_FALSE;
                    break;
                case DepthStencilState::eStencil:
                    dsci.depthTestEnable = VK_FALSE;
                    dsci.depthWriteEnable = VK_FALSE;
                    dsci.stencilTestEnable = VK_TRUE;
                    dsci.depthCompareOp = VK_COMPARE_OP_LESS;
                    dsci.depthBoundsTestEnable = VK_FALSE;
                    break;
                case DepthStencilState::eBoth:
                    dsci.depthTestEnable = dsci.stencilTestEnable = VK_TRUE;
                    dsci.depthWriteEnable = VK_TRUE;
                    break;
                default:
                    std::cerr << "depth stencil state is not described\n";
                    return Result::eFailure;
                    break;
                }
            }

            std::array<VkPipelineShaderStageCreateInfo, 2> ssciArr{};
            result = createShaderModule(info.VS, VK_SHADER_STAGE_VERTEX_BIT, &ssciArr[0]);
            if(Result::eSuccess != result)
            {
                std::cerr << "Failed to create vertex shader module!\n";
                return Result::eFailure;
            }
            result = createShaderModule(info.FS, VK_SHADER_STAGE_FRAGMENT_BIT, &ssciArr[1]);
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create fragment shader module!\n";
                return Result::eFailure;
            }

            { //DescriptorSetLayout

                VkDescriptorSetLayoutCreateInfo descLayoutci{};
                std::vector<VkDescriptorSetLayoutBinding> bindings;

                rpo.layout = info.SRDesc.layout;

                for (const auto &ubBinding : info.SRDesc.layout.getUniformBufferBindings())
                {
                    bindings.emplace_back();
                    auto &b = bindings.back();

                    b.binding = ubBinding;
                    b.descriptorCount = 1;
                    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    b.pImmutableSamplers = nullptr;
                    b.stageFlags = VK_SHADER_STAGE_ALL;
                }

                for (const auto &ctBinding : info.SRDesc.layout.getCombinedTextureBindings())
                {
                    bindings.emplace_back();
                    auto &b = bindings.back();

                    b.binding = ctBinding;
                    b.descriptorCount = 1;
                    b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    b.pImmutableSamplers = nullptr;
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
            { //DescriptorPool
                std::vector<VkDescriptorPoolSize> sizes;

                if(info.SRDesc.layout.getUniformBufferBindings().size() > 0)
                {
                    sizes.emplace_back();
                    sizes.back().descriptorCount = info.SRDesc.layout.getUniformBufferBindings().size() * mMaxFrameNum;
                    sizes.back().type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                }

                if(info.SRDesc.layout.getCombinedTextureBindings().size() > 0)
                {
                    sizes.emplace_back();
                    sizes.back().descriptorCount = info.SRDesc.layout.getCombinedTextureBindings().size() * mMaxFrameNum;
                    sizes.back().type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                }

                VkDescriptorPoolCreateInfo dpci{};
                dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

                dpci.maxSets = info.SRDesc.maxSetCount * mMaxFrameNum;
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

            {//pipeline layout
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

            {// graphics pipeline layout
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

            //won't be used
            for (auto &ssci : ssciArr)
                vkDestroyShaderModule(mDevice, ssci.module, nullptr);
        }

        handle_out = mNextRPHandle++;
        mRPMap.emplace(handle_out, rpo);

        return Result::eSuccess;
    }

    Result Context::createCommandBuffer(const CommandList &commandList, HCommandBuffer &handle_out)
    {
        Result result = Result::eSuccess;

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
        }

        co.mFences.resize(mMaxFrameInFlight);
        {
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
        }


        co.mPresentCompletedSems.resize(mMaxFrameInFlight);
        co.mRenderCompletedSems.resize(mMaxFrameInFlight);

        {
            VkSemaphoreCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            for (size_t i = 0; i < mMaxFrameInFlight; ++i)
            {
                result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &co.mRenderCompletedSems[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create render completed semaphore!\n";
                    return result;
                }
            }

            for (size_t i = 0; i < mMaxFrameInFlight; ++i)
            {
                result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr, &co.mPresentCompletedSems[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create present completed semaphore!\n";
                    return result;
                }
            }
        }

        co.imagesInFlight.resize(mMaxFrameNum, VK_NULL_HANDLE);

        //get internal(public) command info vector
        const auto &cmdList = commandList.getInternalCommandData();

        for (const auto &command : cmdList)
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
            case CommandType::eRenderIndexed:
                if (!std::holds_alternative<CmdRenderIndexed>(command.second))
                    return Result::eFailure;
                result = cmdRenderIndexed(co, std::get<CmdRenderIndexed>(command.second));
                break;
            default:
                std::cerr << "invalid command!\n";
                result = Result::eFailure;
                break;
            }
        }

        handle_out = mNextCBHandle++;
        mCommandBufferMap.emplace(handle_out, co);

        return result;
    }

    Result Context::execute(const HCommandBuffer &handle)
    {
        Result result = Result::eSuccess;

        glfwPollEvents();

        if(mInitializeInfo.debugFlag)
        {
            if (mCommandBufferMap.count(handle) <= 0)
            {
                std::cerr << "invalid commandbuffer handle!\n";
                return Result::eFailure;
            }

            if (mWindowMap.count(handle) <= 0)
            {
                std::cerr << "invalid swapchain handle!\n";
                return Result::eFailure;
            }
        }
        
        auto &co = mCommandBufferMap[handle];
        if(!co.mHRenderDST)
        {
            std::cerr << "renderDST of this command is invalid!\n";
            return Result::eFailure;
        }

        auto &rdsto = mRDSTMap[co.mHRenderDST.value()];


        result = checkVkResult(vkWaitForFences(mDevice, 1, &co.mFences[mCurrentFrame], VK_TRUE, UINT64_MAX));
        if (result != Result::eSuccess)
        {
            std::cerr << "Failed to wait fence!\n";
            return result;
        }


        if(rdsto.mHWindow)
        {
            auto &so = mWindowMap[rdsto.mHWindow.value()];
            uint32_t swapchainImageIndex = 0;

            result = checkVkResult(
                vkAcquireNextImageKHR(
                    mDevice,
                    so.mSwapchain.value(),
                    UINT64_MAX,
                    co.mPresentCompletedSems[mCurrentFrame],
                    VK_NULL_HANDLE,
                    &swapchainImageIndex));

            //std::cout << swapchainImageIndex << " : swapchain image index\n";

            if (result != Result::eSuccess)
            {
                std::cerr << "failed to acquire next swapchain image!\n";
                return result;
            }

            if (co.imagesInFlight[swapchainImageIndex] != VK_NULL_HANDLE)
                vkWaitForFences(mDevice, 1, &co.imagesInFlight[swapchainImageIndex], VK_TRUE, UINT64_MAX);
            co.imagesInFlight[swapchainImageIndex] = co.mFences[mCurrentFrame];

            //submit command
            VkSubmitInfo submitInfo{};
            VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &(co.mCommandBuffers[swapchainImageIndex]);
            submitInfo.pWaitDstStageMask = &waitStageMask;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &co.mPresentCompletedSems[mCurrentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &co.mRenderCompletedSems[mCurrentFrame];
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
            presentInfo.pWaitSemaphores = &co.mRenderCompletedSems[mCurrentFrame];

            result = checkVkResult(vkQueuePresentKHR(mDeviceQueue, &presentInfo));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to present queue!\n";
                return result;
            }
        }
        else
        {
            std::cerr << "this process does not implemented yet!";
            exit(-1);
        }
        //baddest sync process
        //vkQueueWaitIdle(mDeviceQueue);

        mCurrentFrame = (mCurrentFrame + 1) % mMaxFrameInFlight;

        return Result::eSuccess;
    }

    Result Context::cmdBeginRenderPipeline(CommandObject &co, const CmdBeginRenderPipeline &info)
    {
        Result result = Result::eSuccess;

        auto &rpo = mRPMap[info.RPHandle];
        auto &rdsto = mRDSTMap[rpo.mHRenderDST];

        co.mHRPO = info.RPHandle;
        co.mHRenderDST = rpo.mHRenderDST;

        VkCommandBufferBeginInfo commandBI{};
        commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBI.flags = 0;
        commandBI.pInheritanceInfo = nullptr;

        VkRenderPassBeginInfo bi{};

        if (rdsto.mHWindow)
        {
            const auto& swapchain = mWindowMap[rdsto.mHWindow.value()];

            VkClearValue clearValues[2];
            clearValues[0].color = {0.5, 0.5, 0.5, 1.0};
            clearValues[1].depthStencil = {1.f, 0};

            //VkClearValue clearValue = {0.5, 0.5, 0.5, 1.0};

            bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            bi.renderPass = rdsto.mRenderPass.value();
            bi.renderArea.offset = {0, 0};
            bi.renderArea.extent = swapchain.mSwapchainExtent;

            // is this param OK?
            if (rdsto.mDepthTestEnable)
            {
                std::cerr << "this process does not implemented yet!\n";
                //exit(-1);
                bi.clearValueCount = 2;
                bi.pClearValues = clearValues;
            }
            else
            {
                bi.clearValueCount = 1;
                bi.pClearValues = clearValues;
            }
        }
        else
        {
            //TODO: impl of this
            std::cerr << "this process does not implemented yet!\n";
            return Result::eFailure;
        }

        for (size_t i = 0; i < co.mCommandBuffers.size(); ++i)
        {
            auto &command = co.mCommandBuffers[i];
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

    Result Context::cmdEndRenderPipeline(CommandObject &co, const CmdEndRenderPipeline &info)
    {
        Result result;
        for (const auto &command : co.mCommandBuffers)
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

    Result Context::cmdBindVB(CommandObject &co, const CmdBindVB &info)
    {
        if (mBufferMap.count(info.VBHandle) <= 0)
            return Result::eFailure;
        auto &vbo = mBufferMap[info.VBHandle];

        VkDeviceSize offsets[] = {0};
        for (const auto &command : co.mCommandBuffers)
            vkCmdBindVertexBuffers(command, 0, 1, &vbo.mBuffer.value(), offsets);

        return Result::eSuccess;
    }

    Result Context::cmdBindIB(CommandObject &co, const CmdBindIB &info)
    {
        if (mBufferMap.count(info.IBHandle) <= 0)
            return Result::eFailure;
        auto &ibo = mBufferMap[info.IBHandle];

        for (const auto &command : co.mCommandBuffers)
            vkCmdBindIndexBuffer(command, ibo.mBuffer.value(), 0, VK_INDEX_TYPE_UINT32);

        return Result::eSuccess;
    }

    Result Context::cmdBindSRSet(CommandObject &co, const CmdBindSRSet &info)
    {
        Result result = Result::eSuccess;

        if (!co.mHRPO)
        {
            std::cerr << "render pipeline object is not registered yet!\n";
            return Result::eFailure;
        }

        auto &rpo = mRPMap[co.mHRPO.value()];

        if (!rpo.mDescriptorPool)
        {
            std::cerr << "descriptor pool is nothing!\n";
            return Result::eFailure;
        }

        co.mDescriptorSets.resize(mMaxFrameNum);
        std::vector<VkDescriptorBufferInfo> dbi_vec;
        std::vector<VkDescriptorImageInfo> dii_vec;

        // for (const auto &hub : rpo.layout.)
        // {
        //     if (mBufferMap.count(hub) <= 0)
        //     {
        //         std::cerr << "invalid uniform buffer handle!\n";
        //         return Result::eFailure;
        //     }
        //     auto &ub = mBufferMap[hub];
        //     dbi_vec.emplace_back();
        //     dbi_vec.back().buffer = ub.mBuffer.value();
        //     dbi_vec.back().offset = 0;
        //     dbi_vec.back().range = VK_WHOLE_SIZE;
        // }

        // for (auto &hct : info.SRSet.combinedTexture)
        // {
        //     if (mImageMap.count(hct) <= 0)
        //     {
        //         std::cerr << "invalid combined texture handle!\n";
        //         return Result::eFailure;
        //     }

        //     auto &ct = mImageMap[hct];
        //     dii_vec.emplace_back();
        //     dii_vec.back().imageView = ct.mView.value();
        //     dii_vec.back().sampler = ct.mSampler.value();
        //     dii_vec.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // }

        std::vector<VkDescriptorSetLayout> layouts(mMaxFrameNum, rpo.mDescriptorSetLayout.value());
        std::vector<VkWriteDescriptorSet> writeDescriptors;

        {
            VkDescriptorSetAllocateInfo dsai{};
            dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsai.descriptorPool = rpo.mDescriptorPool.value();
            dsai.descriptorSetCount = co.mDescriptorSets.size();
            dsai.pSetLayouts = layouts.data();
            result = checkVkResult(vkAllocateDescriptorSets(mDevice, &dsai, co.mDescriptorSets.data()));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to allocate descriptor set\n";
                return result;
            }

            for (size_t i = 0; i < co.mDescriptorSets.size(); ++i)
            {
                writeDescriptors.clear();
                writeDescriptors.reserve
                (
                    rpo.layout.getUniformBufferBindings().size() +
                    rpo.layout.getCombinedTextureBindings().size()
                );

                for (const auto &dub: info.SRSet.getUniformBuffers())
                {
                    auto &ubo = mBufferMap[dub.second];
                    dbi_vec.emplace_back();
                    dbi_vec.back().buffer = ubo.mBuffer.value();
                    dbi_vec.back().offset = 0;
                    dbi_vec.back().range = VK_WHOLE_SIZE;

                    writeDescriptors.emplace_back(VkWriteDescriptorSet{});
                    writeDescriptors.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptors.back().dstBinding = dub.first;
                    writeDescriptors.back().dstArrayElement = 0;
                    writeDescriptors.back().descriptorCount = 1;
                    writeDescriptors.back().descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    writeDescriptors.back().pBufferInfo = &dbi_vec.back();
                    writeDescriptors.back().dstSet = co.mDescriptorSets[i];
                }

                for (const auto &dct : info.SRSet.getCombinedTextures())
                {
                    auto &cto = mImageMap[dct.second];
                    dii_vec.emplace_back();
                    dii_vec.back().imageView = cto.mView.value();
                    dii_vec.back().sampler = cto.mSampler.value();
                    dii_vec.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    writeDescriptors.emplace_back(VkWriteDescriptorSet{});
                    writeDescriptors.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptors.back().dstBinding = dct.first;
                    writeDescriptors.back().dstArrayElement = 0;
                    writeDescriptors.back().descriptorCount = 1;
                    writeDescriptors.back().descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptors.back().pImageInfo = &dii_vec.back();
                    writeDescriptors.back().dstSet = co.mDescriptorSets[i];

                }

                vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
            }
        }

        for (size_t i = 0; i < co.mCommandBuffers.size(); ++i)
        {
            vkCmdBindDescriptorSets(
                co.mCommandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                rpo.mPipelineLayout.value(),
                0,
                1,
                &co.mDescriptorSets[i],
                0,
                nullptr);
        }

        return Result::eSuccess;
    }

    Result Context::cmdRenderIndexed(CommandObject &co, const CmdRenderIndexed &info)
    {
        for (const auto &command : co.mCommandBuffers)
            vkCmdDrawIndexed(
                command,
                info.indexCount,
                info.instanceCount,
                info.firstIndex,
                info.vertexOffset,
                info.firstInstance);

        return Result::eSuccess;
    }

    Result Context::cmdRender(CommandObject &co, const CmdRender &info)
    {

        for (const auto &command : co.mCommandBuffers)
            vkCmdDraw(
                command,
                info.vertexCount,
                info.instanceCount,
                info.vertexOffset,
                info.firstInstance);

        return Result::eSuccess;
    }

   
}; // namespace Cutlass