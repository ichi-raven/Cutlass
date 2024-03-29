#include "../include/Context.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <variant>
#include <vector>

#include "../include/Event.hpp"
#include "../include/ThirdParty/imgui.h"
#include "../include/ThirdParty/imgui_impl_glfw.h"
#include "../include/ThirdParty/imgui_impl_vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/ThirdParty/stb_image.h"

#define GetInstanceProcAddr(FuncName)               \
    m##FuncName = reinterpret_cast<PFN_##FuncName>( \
        vkGetInstanceProcAddr(mInstance, #FuncName))

namespace Cutlass
{
#define GetInstanceProcAddr(FuncName)               \
    m##FuncName = reinterpret_cast<PFN_##FuncName>( \
        vkGetInstanceProcAddr(mInstance, #FuncName))

    static VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objactTypes,
        uint64_t object, size_t location, int32_t messageCode,
        const char* pLayerPrefix, const char* pMessage, void* pUserData)
    {
        VkBool32 ret = VK_FALSE;
        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ||
            flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            ret = VK_TRUE;

        if (pLayerPrefix)
            std::cerr << "[" << pLayerPrefix << "] ";

        std::cerr << pMessage << std::endl;

        return ret;
    }

    static void ImGui_check_vk_result(VkResult err)
    {
        if (err == 0)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
        if (err < 0)
            abort();
    }

    Context::Context()
    {
        mIsInitialized = false;
        mMaxFrame      = 0;
        mNextWindowHandle.setID(1);
        mNextBufferHandle.setID(1);
        mNextTextureHandle.setID(1);
        mNextRenderPassHandle.setID(1);
        mNextGPHandle.setID(1);
        mNextCBHandle.setID(1);
        mAppName = std::string("CutlassApp");
    }

    Context::Context(std::string_view appName, bool debugFlag, Result& result_out)
    {
        mIsInitialized = false;
        mMaxFrame      = 0;
        mNextWindowHandle.setID(1);
        mNextBufferHandle.setID(1);
        mNextTextureHandle.setID(1);
        mNextRenderPassHandle.setID(1);
        mNextGPHandle.setID(1);
        mNextCBHandle.setID(1);

        result_out = initialize(appName, debugFlag);
    }

    Context::~Context()
    {
        if (mIsInitialized)
        {
            // std::cerr << "You forgot destroying context explicitly!\n";
            destroy();
        }
    }

    Result Context::initialize(std::string_view appName, bool debugFlag)
    {
        Result result;

        mAppName   = std::string(appName);
        mDebugFlag = debugFlag;

        std::cerr << "initializing started...\n";

        // GLFW initialization

        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW!\n";
            exit(-1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        std::cerr << "GLFW initialized\n";

        // instance
        result = createInstance();
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::cerr << "created VkInstance\n";

        if (mDebugFlag)
        {
            result = enableDebugReport();
            if (Result::eSuccess != result)
            {
                return result;
            }
            std::cerr << "debug report enabled\n";
        }

        // select physical device
        result = selectPhysicalDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "selected Physical Device\n";

        // search graphics queue
        result = searchGraphicsQueueIndex();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "found index of GraphicsQueue\n";

        // logical device
        result = createDevice();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkDevice\n";

        // command pool
        result = createCommandPool();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkCommandPool\n";

        // descriptor pool
        result = addDescriptorPool();
        if (Result::eSuccess != result)
        {
            return result;
        }
        std::cerr << "created VkDescriptorPool\n";

        std::cerr << "all initialize processes succeeded\n";
        mIsInitialized = true;

        return Result::eSuccess;
    }

    Result Context::destroy()
    {
        if (!mIsInitialized)
        {
            std::cerr << "this context was not initialized!\n";
            return Result::eFailure;
        }

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
        std::cerr << "destroyed user allocated buffers(size : " << mBufferMap.size()
                  << ")\n";
        mBufferMap.clear();

        for (auto& e : mImageMap)
        {
            if (e.second.mView)
                vkDestroyImageView(mDevice, e.second.mView.value(), nullptr);

            if (e.second.usage ==
                TextureUsage::eSwapchainImage)  // avoid destroying SwapchainImage
                continue;

            if (e.second.mImage)
                vkDestroyImage(mDevice, e.second.mImage.value(), nullptr);
            if (e.second.mMemory)
                vkFreeMemory(mDevice, e.second.mMemory.value(), nullptr);

            if (e.second.mSampler)
                vkDestroySampler(mDevice, e.second.mSampler.value(), nullptr);
        }

        std::cerr << "destroyed user allocated textures(size : " << mImageMap.size()
                  << ")\n";
        std::cerr << "destroyed user allocated sampler\n";
        mImageMap.clear();

        for (auto& e : mRPMap)
        {
            for (auto& f : e.second.mFences)
                vkDestroyFence(mDevice, f, nullptr);
            for (const auto& pcSem : e.second.mPresentCompletedSems)
                vkDestroySemaphore(mDevice, pcSem, nullptr);
            for (const auto& rcSem : e.second.mRenderCompletedSems)
                vkDestroySemaphore(mDevice, rcSem, nullptr);

            for (auto& f : e.second.mFramebuffers)
                vkDestroyFramebuffer(mDevice, f.value(), nullptr);

            if (e.second.mRenderPass)
                vkDestroyRenderPass(mDevice, e.second.mRenderPass.value(), nullptr);
        }

        for (auto& e : mGPMap)
        {
            for (const auto& dsl : e.second.mDescriptorSetLayouts)
                vkDestroyDescriptorSetLayout(mDevice, dsl, nullptr);
            if (e.second.mPipelineLayout)
                vkDestroyPipelineLayout(mDevice, e.second.mPipelineLayout.value(),
                                        nullptr);
            if (e.second.mPipeline)
                vkDestroyPipeline(mDevice, e.second.mPipeline.value(), nullptr);
        }

        for (auto& co : mCommandBufferMap)
        {
            if (!co.second.mDescriptorSets.empty())
            {
                for (const auto& ds_vec : co.second.mDescriptorSets)
                {
                    std::vector<VkDescriptorSet> sets;
                    sets.reserve(ds_vec.size());
                    for (const auto& e : ds_vec)
                        sets.emplace_back(e.value());

                    if (!sets.empty())
                        vkFreeDescriptorSets(
                            mDevice, mDescriptorPools[co.second.mDescriptorPoolIndex].second,
                            sets.size(), sets.data());
                    

                    mDescriptorPools[co.second.mDescriptorPoolIndex]
                        .first.uniformBufferCount -= co.second.mUBCount;
                    mDescriptorPools[co.second.mDescriptorPoolIndex]
                        .first.combinedTextureCount -= co.second.mCTCount;
                }
            }

            vkFreeCommandBuffers(mDevice, mCommandPool,
                                 uint32_t(co.second.mCommandBuffers.size()),
                                 co.second.mCommandBuffers.data());
        }

        std::cerr << "destroyed command buffers(size : " << mCommandBufferMap.size()
                  << ")\n";
        mCommandBufferMap.clear();
        // for(auto& e : mSamplerMap)
        //{
        //     vkDestroySampler(mDevice, e.second, nullptr);
        // }
        // mSamplerMap.clear();

        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        std::cerr << "destroyed command pool\n";

        for (auto& dp_pair : mDescriptorPools)
        {
            vkDestroyDescriptorPool(mDevice, dp_pair.second, nullptr);
        }

        for (auto& so : mWindowMap)
        {
            // for (auto& f : so.second.mFences)
            //     vkDestroyFence(mDevice, f, nullptr);
            // for (const auto& pcSem : so.second.mPresentCompletedSems)
            //     vkDestroySemaphore(mDevice, pcSem, nullptr);
            // for (const auto& rcSem : so.second.mRenderCompletedSems)
            //     vkDestroySemaphore(mDevice, rcSem, nullptr);

            if (so.second.useImGui)
            {
                if (mImGuiDescriptorPool)
                    vkDestroyDescriptorPool(mDevice, mImGuiDescriptorPool.value(), nullptr);

                if (mImGuiRenderPass)
                    vkDestroyRenderPass(mDevice, mImGuiRenderPass.value(), nullptr);

                ImGui_ImplVulkan_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                std::cerr << "shutdown ImGui\n";
            }

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
        std::cerr << "destroyed semaphores\n";
        std::cerr << "destroyed fences\n";
        std::cerr << "destroyed all swapchains and surfaces\n";

        if (mDebugFlag)
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

    Result Context::destroyBuffer(const HBuffer& handle)
    {
        Result result = Result::eSuccess;

        if (mDebugFlag && mBufferMap.count(handle) <= 0)
        {
            std::cerr << "invalid buffer handle!\n";
            return Result::eFailure;
        }

        auto& bo = mBufferMap[handle];

        // wait queue before
        vkQueueWaitIdle(mDeviceQueue);

        if (bo.mBuffer)
            vkDestroyBuffer(mDevice, bo.mBuffer.value(), nullptr);
        if (bo.mMemory)
            vkFreeMemory(mDevice, bo.mMemory.value(), nullptr);

        mBufferMap.erase(handle);

        return result;
    }

    Result Context::destroyTexture(const HTexture& handle)
    {
        Result result = Result::eSuccess;

        if (mDebugFlag && mImageMap.count(handle) <= 0)
        {
            std::cerr << "invalid texture handle!\n";
            return Result::eFailure;
        }

        auto& io = mImageMap[handle];

        // wait queue before
        vkQueueWaitIdle(mDeviceQueue);

        if (io.mView)
            vkDestroyImageView(mDevice, io.mView.value(), nullptr);

        if (io.usage ==
            TextureUsage::eSwapchainImage)  // avoid destroying SwapchainImage
            return result;

        if (io.mImage)
            vkDestroyImage(mDevice, io.mImage.value(), nullptr);
        if (io.mMemory)
            vkFreeMemory(mDevice, io.mMemory.value(), nullptr);

        if (io.mSampler)
            vkDestroySampler(mDevice, io.mSampler.value(), nullptr);

        mImageMap.erase(handle);

        return result;
    }

    // Result Context::destroyRenderPass(const HRenderPass& handle)
    // {
    //     Result result = Result::eSuccess;

    //     if (mDebugFlag && mRPMap.count(handle) <= 0)
    //     {
    //         std::cerr << "invalid renderPass handle!\n";
    //         return Result::eFailure;
    //     }

    //     auto& rpo = mRPMap[handle];

    //     for (auto& f : rpo.mFramebuffers)
    //         vkDestroyFramebuffer(mDevice, f.value(), nullptr);

    //     if (rpo.mRenderPass)
    //         vkDestroyRenderPass(mDevice, rpo.mRenderPass.value(), nullptr);

    //     mRPMap.erase(handle);

    //     return result;
    // }

    Result Context::destroyGraphicsPipeline(const HGraphicsPipeline& handle)
    {
        Result result = Result::eSuccess;

        if (mDebugFlag && mGPMap.count(handle) <= 0)
        {
            std::cerr << "invalid render pipeline handle!\n";
            return Result::eFailure;
        }

        auto& gpo = mGPMap[handle];

        if (mDebugFlag && mRPMap.count(gpo.mHRenderPass) <= 0)
        {
            std::cerr << "invalid renderPass handle!\n";
            return Result::eFailure;
        }

        auto& rpo = mRPMap[gpo.mHRenderPass];

        // stop queue before
        vkQueueWaitIdle(mDeviceQueue);

        {  // ImGui
            if (mImGuiDescriptorPool)
                vkDestroyDescriptorPool(mDevice, mImGuiDescriptorPool.value(), nullptr);

            if (mImGuiRenderPass)
                vkDestroyRenderPass(mDevice, mImGuiRenderPass.value(), nullptr);

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        for (auto& f : rpo.mFramebuffers)
            vkDestroyFramebuffer(mDevice, f.value(), nullptr);

        if (rpo.mRenderPass)
            vkDestroyRenderPass(mDevice, rpo.mRenderPass.value(), nullptr);

        mRPMap.erase(gpo.mHRenderPass);

        for (const auto& dsl : gpo.mDescriptorSetLayouts)
            vkDestroyDescriptorSetLayout(mDevice, dsl, nullptr);

        // if (gpo.mDescriptorPool)
        //    vkDestroyDescriptorPool(mDevice, gpo.mDescriptorPool.value(),
        //    nullptr);
        if (gpo.mPipelineLayout)
            vkDestroyPipelineLayout(mDevice, gpo.mPipelineLayout.value(), nullptr);
        if (gpo.mPipeline)
            vkDestroyPipeline(mDevice, gpo.mPipeline.value(), nullptr);

        mGPMap.erase(handle);

        return result;
    }

    Result Context::destroyCommandBuffer(const HCommandBuffer& handle)
    {
        Result result = Result::eSuccess;

        if (mDebugFlag && mCommandBufferMap.count(handle) <= 0)
        {
            std::cerr << "invalid command buffer handle!\n";
            return Result::eFailure;
        }

        auto& co = mCommandBufferMap[handle];

        // stop queue before
        vkQueueWaitIdle(mDeviceQueue);

        if (!co.mDescriptorSets.empty())
        {
            for (const auto& ds_vec : co.mDescriptorSets)
            {
                std::vector<VkDescriptorSet> sets;
                sets.reserve(ds_vec.size());
                for (const auto& e : ds_vec)
                    sets.emplace_back(e.value());

                if (!sets.empty())
                    vkFreeDescriptorSets(mDevice,
                                     mDescriptorPools[co.mDescriptorPoolIndex].second,
                                     sets.size(), sets.data());
                mDescriptorPools[co.mDescriptorPoolIndex].first.uniformBufferCount -=
                    co.mUBCount;
                mDescriptorPools[co.mDescriptorPoolIndex].first.combinedTextureCount -=
                    co.mCTCount;
            }
        }

        vkFreeCommandBuffers(mDevice, mCommandPool,
                             uint32_t(co.mCommandBuffers.size()),
                             co.mCommandBuffers.data());

        mCommandBufferMap.erase(handle);

        return result;
    }

    Result Context::destroyWindow(const HWindow& handle)
    {
        Result result = Result::eSuccess;

        if (mDebugFlag && mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid window handle!\n";
            return Result::eFailure;
        }

        // destroy ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        auto& wo = mWindowMap[handle];

        if (wo.mSwapchain)
            vkDestroySwapchainKHR(mDevice, wo.mSwapchain.value(), nullptr);

        if (wo.mSurface)
            vkDestroySurfaceKHR(mInstance, wo.mSurface.value(), nullptr);

        if (wo.mpWindow)
            glfwDestroyWindow(wo.mpWindow.value());

        return result;
    }

    Result Context::checkVkResult(VkResult vkResult)
    {
        switch (vkResult)
        {
            case VK_SUCCESS:
                return Result::eSuccess;
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                std::cerr << "swapchain is resized!\n";
                return Result::eSuccess;
                break;
            default:
                std::cerr << "ERROR!\nvulkan error : " << vkResult << "\n";
                return Result::eFailure;  // TODO: error handling
                break;
        }

        return Result::eSuccess;
    }

    Result Context::createInstance()
    {
        Result result;

        std::vector<const char*> extensions;
        VkApplicationInfo appInfo{};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = mAppName.c_str();
        appInfo.pEngineName      = ENGINE_NAME;
        appInfo.apiVersion       = VK_API_VERSION_1_2;
        appInfo.engineVersion    = VK_MAKE_VERSION(version[0], version[1], version[2]);

        // get extention properties
        std::vector<VkExtensionProperties> props;

        {
            uint32_t count = 0;
            result         = checkVkResult(
                        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }

            props.resize(count);
            result = checkVkResult(
                vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data()));
            if (Result::eSuccess != result)
            {
                return result;
            }

            std::cerr << "enabled extensions : \n";
            for (const auto& v : props)
            {
                extensions.push_back(v.extensionName);
                std::cerr << v.extensionName << "\n";
            }
        }

        VkInstanceCreateInfo ci{};
        ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.enabledExtensionCount   = uint32_t(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.pApplicationInfo        = &appInfo;

        const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
        if (mDebugFlag)
        {
            ci.enabledLayerCount   = 1;
            ci.ppEnabledLayerNames = layers;
        }
        else
        {
            ci.enabledLayerCount   = 0;
            ci.ppEnabledLayerNames = nullptr;
        }

        result = checkVkResult(vkCreateInstance(&ci, nullptr, &mInstance));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to enable debug layer! creating VkInstance without "
                         "debug layer...\n";
            ci.enabledLayerCount   = 0;
            ci.ppEnabledLayerNames = nullptr;
            result                 = checkVkResult(vkCreateInstance(&ci, nullptr, &mInstance));
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
        result =
            checkVkResult(vkEnumeratePhysicalDevices(mInstance, &devCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkPhysicalDevice> physDevs(devCount);
        result = checkVkResult(
            vkEnumeratePhysicalDevices(mInstance, &devCount, physDevs.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::cerr << "Physical Device number : " << physDevs.size() << "\n";

        // select suitable device
        for (const auto& pd : physDevs)
        {
            std::optional<uint32_t> indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount,
                                                     queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
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

        // get physical memory properties
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
            result         = checkVkResult(vkEnumerateDeviceExtensionProperties(
                        mPhysDev, nullptr, &count, nullptr));
            if (Result::eSuccess != result)
            {
                return result;
            }

            devExtProps.resize(count);
            result = checkVkResult(vkEnumerateDeviceExtensionProperties(
                mPhysDev, nullptr, &count, devExtProps.data()));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        {
            const float defaultQueuePriority(1.0f);
            VkDeviceQueueCreateInfo devQueueCI{};
            devQueueCI.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            devQueueCI.queueFamilyIndex = mGraphicsQueueIndex;
            devQueueCI.queueCount       = 1;
            devQueueCI.pQueuePriorities = &defaultQueuePriority;

            std::vector<const char*> extensions;
            for (const auto& v : devExtProps)
            {
                if (strcmp(v.extensionName, "VK_KHR_buffer_device_address"))
                    extensions.push_back(v.extensionName);
            }

            VkDeviceCreateInfo ci{};
            ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            ci.pQueueCreateInfos       = &devQueueCI;
            ci.queueCreateInfoCount    = 1;
            ci.ppEnabledExtensionNames = extensions.data();
            ci.enabledExtensionCount   = uint32_t(extensions.size());

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
            ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.queueFamilyIndex = mGraphicsQueueIndex;
            ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            result = checkVkResult(
                vkCreateCommandPool(mDevice, &ci, nullptr, &mCommandPool));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }
        return Result::eSuccess;
    }

    Result Context::addDescriptorPool()
    {
        Result result = Result::eSuccess;

        std::array<VkDescriptorPoolSize, 2> sizes;

        sizes[0].descriptorCount = DescriptorPoolInfo::poolUBSize;
        sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        sizes.back().descriptorCount = DescriptorPoolInfo::poolCTSize;
        sizes.back().type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorPoolCreateInfo dpci{};
        dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        dpci.maxSets =
            DescriptorPoolInfo::poolUBSize + DescriptorPoolInfo::poolCTSize;

        dpci.poolSizeCount = static_cast<uint32_t>(sizes.size());
        dpci.pPoolSizes    = sizes.data();
        {
            VkDescriptorPool descriptorPool;
            result = checkVkResult(
                vkCreateDescriptorPool(mDevice, &dpci, nullptr, &descriptorPool));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to create Descriptor Pool\n";
                return result;
            }
            mDescriptorPools.emplace_back(DescriptorPoolInfo(), descriptorPool);
        }


        return result;
    }

    Result Context::createSurface(WindowObject& wo)
    {
        Result result;

        VkSurfaceKHR surface;
        result = checkVkResult(glfwCreateWindowSurface(mInstance, wo.mpWindow.value(),
                                                       nullptr, &surface));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to create window surface!\n";
            return result;
        }
        wo.mSurface = surface;

        result = selectSurfaceFormat(wo, VK_FORMAT_B8G8R8A8_UNORM);
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to select surface format!\n";
            return result;
        }

        result = checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            mPhysDev, wo.mSurface.value(), &wo.mSurfaceCaps));
        if (Result::eSuccess != result)
        {
            std::cerr << "Failed to get physical device surface capability!\n";
            return result;
        }

        VkBool32 isSupport;
        result = checkVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(
            mPhysDev, mGraphicsQueueIndex, wo.mSurface.value(), &isSupport));
        if (Result::eSuccess != result)
        {
            return result;
        }

        return Result::eSuccess;
    }

    Result Context::selectSurfaceFormat(WindowObject& wo, VkFormat format)
    {
        Result result;

        uint32_t surfaceFormatCount = 0;  // get count of formats
        result                      = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(
                                 mPhysDev, wo.mSurface.value(), &surfaceFormatCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkSurfaceFormatKHR> formats(
            surfaceFormatCount);  // get actual format
        result = checkVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(
            mPhysDev, wo.mSurface.value(), &surfaceFormatCount, formats.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        // search matched format
        for (const auto& f : formats)
        {
            if (f.format == format)
            {
                wo.mSurfaceFormat = f;
            }
        }

        return Result::eSuccess;
    }

    Result Context::createSwapchain(WindowObject& wo, bool vsync)
    {
        Result result;
        mMaxFrame = std::max(wo.mMaxFrameNum, mMaxFrame);

        if (wo.mMaxFrameNum < wo.mSurfaceCaps.minImageCount)
        {
            std::cerr << "required frame count is lower than minimum surface "
                         "frame count!\n";
            std::cerr << "minimum frame count : " << wo.mSurfaceCaps.minImageCount
                      << "\n";
            return Result::eFailure;
        }

        if (wo.mSurfaceCaps.maxImageCount &&
            wo.mMaxFrameNum > wo.mSurfaceCaps.maxImageCount)
        {
            std::cerr << "required frame count is upper than maximum surface "
                         "frame count!\n";
            std::cerr << "maximum frame count : " << wo.mSurfaceCaps.maxImageCount
                      << "\n";
            return Result::eFailure;
        }

        auto& extent = wo.mSurfaceCaps.currentExtent;
        if (extent.width <= 0u || extent.height <= 0u)
        {
            // ignore invalid param and use window size
            int width, height;
            glfwGetWindowSize(wo.mpWindow.value(), &width, &height);
            extent.width  = static_cast<uint32_t>(width);
            extent.height = static_cast<uint32_t>(height);
        }

        wo.mPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        uint32_t queueFamilyIndices[] = {mGraphicsQueueIndex};
        VkSwapchainCreateInfoKHR ci{};
        ci.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface               = wo.mSurface.value();
        ci.minImageCount         = wo.mSurfaceCaps.minImageCount;
        ci.imageFormat           = wo.mSurfaceFormat.format;
        ci.imageColorSpace       = wo.mSurfaceFormat.colorSpace;
        ci.imageExtent           = extent;
        ci.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.preTransform          = wo.mSurfaceCaps.currentTransform;
        ci.imageArrayLayers      = 1;
        ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        ci.queueFamilyIndexCount = 0;
        ci.presentMode =
            vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        ci.oldSwapchain   = VK_NULL_HANDLE;
        ci.clipped        = VK_TRUE;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        {
            VkSwapchainKHR swapchain;
            result =
                checkVkResult(vkCreateSwapchainKHR(mDevice, &ci, nullptr, &swapchain));
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

    Result Context::createSwapchainImages(WindowObject& wo)
    {
        Result result;

        uint32_t imageCount;
        result = checkVkResult(vkGetSwapchainImagesKHR(mDevice, wo.mSwapchain.value(),
                                                       &imageCount, nullptr));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkImage> swapchainImages(imageCount);
        result = checkVkResult(vkGetSwapchainImagesKHR(
            mDevice, wo.mSwapchain.value(), &imageCount, swapchainImages.data()));
        if (Result::eSuccess != result)
        {
            return result;
        }

        std::vector<VkImageView> swapchainViews(imageCount);

        for (size_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo ci{};
            ci.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType   = VK_IMAGE_VIEW_TYPE_2D;
            ci.format     = wo.mSurfaceFormat.format;
            ci.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            };
            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            ci.image            = swapchainImages[i];
            result              = checkVkResult(
                             vkCreateImageView(mDevice, &ci, nullptr, &swapchainViews[i]));
            if (Result::eSuccess != result)
            {
                std::cerr << "failed to create swapchain image views!\n";
                return result;
            }
        }

        for (size_t i = 0; i < imageCount; ++i)
        {
            ImageObject io;
            io.mImage         = swapchainImages[i];
            io.mView          = swapchainViews[i];
            io.usage          = TextureUsage::eSwapchainImage;
            io.mIsHostVisible = false;
            io.extent         = {wo.mSwapchainExtent.width, wo.mSwapchainExtent.height,
                         static_cast<uint32_t>(1)};
            io.format         = wo.mSurfaceFormat.format;
            io.currentLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            wo.mHSwapchainImages.emplace_back(mNextTextureHandle++);
            mImageMap.emplace(wo.mHSwapchainImages.back(), io);
        }

        return Result::eSuccess;
    }

    Result Context::enableDebugReport()
    {
        GetInstanceProcAddr(vkCreateDebugReportCallbackEXT);
        GetInstanceProcAddr(vkDebugReportMessageEXT);
        GetInstanceProcAddr(vkDestroyDebugReportCallbackEXT);

        VkDebugReportFlagsEXT flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT drcci{};
        drcci.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        drcci.flags       = flags;
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

    uint32_t Context::getMemoryTypeIndex(uint32_t requestBits,
                                         VkMemoryPropertyFlags requestProps) const
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

    Result Context::createWindow(const WindowInfo& info, HWindow& handle_out)
    {
        Result result = Result::eSuccess;
        WindowObject wo;
        wo.mMaxFrameNum      = info.frameCount;
        wo.mMaxFrameInFlight = std::max(1, static_cast<int>(info.frameCount) - 1);
        wo.mCurrentFrame     = 0;

        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        if (info.fullScreen)
            wo.mpWindow = std::make_optional(
                glfwCreateWindow(info.width, info.height, info.windowName.c_str(),
                                 glfwGetPrimaryMonitor(), nullptr));
        else
            wo.mpWindow = std::make_optional(glfwCreateWindow(
                info.width, info.height, info.windowName.c_str(), nullptr, nullptr));

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

        result = createSwapchain(wo, info.vsync);
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
        std::cerr << "created swapchain depth buffer\n";

        // result = createSyncObjects(wo);
        // if (Result::eSuccess != result)
        // {
        //     return result;
        // }
        // std::cerr << "created swapchain sync objects\n";

        wo.useImGui = info.useImGui;

        // set swapchain object
        handle_out = mNextWindowHandle++;
        mWindowMap.emplace(handle_out, wo);

        return result;
    }

    Result Context::getWindowSize(const HWindow& handle, uint32_t& width,
                                  uint32_t& height)
    {
        if (mDebugFlag && mWindowMap.count(handle) <= 0)
        {
            std::cerr << "Invalid window handle!\n";
            return Result::eFailure;
        }

        auto& wo = mWindowMap[handle];
        width    = wo.mSwapchainExtent.width;
        height   = wo.mSwapchainExtent.height;
        return Result::eSuccess;
    }

    Result Context::createBuffer(const BufferInfo& info, HBuffer& handle_out)
    {
        const auto out = mNextBufferHandle++;
        auto&& result  = createBuffer(info, out);
        if (Result::eSuccess != result)
            return result;
        handle_out = out;
        return result;
    }

    Result Context::updateBuffer(const BufferInfo& info, const HBuffer& handle)
    {
        auto&& result = destroyBuffer(handle);
        if (Result::eSuccess != result)
            return result;
        return createBuffer(info, handle);
    }

    Result Context::createBuffer(const BufferInfo& info, const HBuffer& handle)
    {
        Result result = Result::eFailure;
        BufferObject bo;

        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

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

            ci.size  = info.size;
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
                fb = static_cast<VkMemoryPropertyFlagBits>(
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                bo.mIsHostVisible = true;
            }
            else
            {
                fb                = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                bo.mIsHostVisible = false;
            }

            VkMemoryRequirements reqs;
            vkGetBufferMemoryRequirements(mDevice, bo.mBuffer.value(), &reqs);
            VkMemoryAllocateInfo ai{};
            ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize  = reqs.size;
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
            result = checkVkResult(
                vkBindBufferMemory(mDevice, bo.mBuffer.value(), bo.mMemory.value(), 0));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        if (mBufferMap.count(handle) <= 0)
            mBufferMap.emplace(handle, bo);
        else
            mBufferMap[handle] = bo;

        return Result::eSuccess;
    }

    Result Context::writeBuffer(const size_t size, const void* const pData,
                                const HBuffer& handle)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eFailure;
        if (mBufferMap.count(handle) <= 0)
            return result;

        BufferObject& bo = mBufferMap[handle];

        void* p;  // mapping dst address

        result = checkVkResult(
            vkMapMemory(mDevice, bo.mMemory.value(), 0, VK_WHOLE_SIZE, 0, &p));
        if (result != Result::eSuccess)
            return result;
        memcpy(p, pData, size);
        vkUnmapMemory(mDevice, bo.mMemory.value());

        return Result::eSuccess;
    }

    Result Context::createTexture(const TextureInfo& info, HTexture& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result;
        ImageObject io;
        {
            VkImageCreateInfo ci{};

            ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.pNext = nullptr;

            switch (info.format)
            {
                case ResourceType::eUnorm:
                    io.format         = VK_FORMAT_R8_UNORM;
                    io.mSizeOfChannel = 1;
                    break;
                case ResourceType::eFloat32:
                    if (info.usage == TextureUsage::eDepthStencilTarget)
                        io.format = VK_FORMAT_D32_SFLOAT;
                    else
                        io.format = VK_FORMAT_R32_SFLOAT;
                    io.mSizeOfChannel = 4;
                    break;
                case ResourceType::eUint32:
                    io.format         = VK_FORMAT_R32_UINT;
                    io.mSizeOfChannel = 4;
                    break;
                case ResourceType::eInt32:
                    io.format         = VK_FORMAT_R32_SINT;
                    io.mSizeOfChannel = 4;
                    break;

                case ResourceType::eUNormVec2:
                    io.format         = VK_FORMAT_R8G8_UNORM;
                    io.mSizeOfChannel = 2;
                    break;
                case ResourceType::eF32Vec2:
                    io.format         = VK_FORMAT_R32G32_SFLOAT;
                    io.mSizeOfChannel = 8;
                    break;
                case ResourceType::eU32Vec2:
                    io.format         = VK_FORMAT_R32G32_UINT;
                    io.mSizeOfChannel = 8;
                    break;
                case ResourceType::eS32Vec2:
                    io.format         = VK_FORMAT_R32G32_SINT;
                    io.mSizeOfChannel = 8;
                    break;

                case ResourceType::eUNormVec3:
                    io.format         = VK_FORMAT_R8G8B8_UNORM;
                    io.mSizeOfChannel = 3;
                    break;
                case ResourceType::eF32Vec3:
                    io.format         = VK_FORMAT_R32G32B32_SFLOAT;
                    io.mSizeOfChannel = 12;
                    break;
                case ResourceType::eU32Vec3:
                    io.format         = VK_FORMAT_R32G32B32_UINT;
                    io.mSizeOfChannel = 12;
                    break;
                case ResourceType::eS32Vec3:
                    io.format         = VK_FORMAT_R32G32B32_SINT;
                    io.mSizeOfChannel = 12;
                    break;

                case ResourceType::eUNormVec4:
                    io.format         = VK_FORMAT_R8G8B8A8_UNORM;
                    io.mSizeOfChannel = 4;
                    break;
                case ResourceType::eF32Vec4:
                    io.format         = VK_FORMAT_R32G32B32A32_SFLOAT;
                    io.mSizeOfChannel = 16;
                    break;
                case ResourceType::eU32Vec4:
                    io.format         = VK_FORMAT_R32G32B32A32_UINT;
                    io.mSizeOfChannel = 16;
                    break;
                case ResourceType::eS32Vec4:
                    io.format         = VK_FORMAT_R32G32B32A32_SINT;
                    io.mSizeOfChannel = 16;
                    break;
                default:
                    std::cerr << "invalid type of pixel!\n";
                    return Result::eFailure;
                    break;
            }

            ci.format = io.format;

            switch (info.dimension)
            {
                case Dimension::e2D:
                    ci.imageType = VK_IMAGE_TYPE_2D;
                    ci.extent    = {uint32_t(info.width), uint32_t(info.height), 1};
                    if (info.depth != 1)
                        std::cerr << "ignored invalid depth param\n";
                    break;
                    // case TextureDimention::e3D:
                    //     ci.imageType = VK_IMAGE_TYPE_3D;
                    //     ci.extent = {uint32_t(info.width),
                    //     uint32_t(info.height), uint32_t(info.depth)}; break;
                default:
                    std::cerr << "invalid dimention of texture!\n";
                    return Result::eFailure;
                    break;
            }

            switch (info.usage)
            {
                case TextureUsage::eShaderResource:
                    ci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                    io.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    break;
                case TextureUsage::eColorTarget:
                    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                    io.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    break;
                case TextureUsage::eDepthStencilTarget:
                    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                    io.currentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                    break;
                case TextureUsage::eUnordered:
                    ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    io.currentLayout = VK_IMAGE_LAYOUT_GENERAL;
                    break;
                default:
                    std::cerr << "invalid usage!\n";
                    return Result::eFailure;
                    break;
            }

            io.usage = info.usage;

            ci.arrayLayers   = 1;
            ci.mipLevels     = 1;
            ci.samples       = VK_SAMPLE_COUNT_1_BIT;
            ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            io.extent        = ci.extent;

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
        ai.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        // decide memory type
        VkMemoryPropertyFlagBits fb;
        if (info.isHostVisible)
        {
            fb                = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            io.mIsHostVisible = false;
        }
        else
        {
            fb = static_cast<VkMemoryPropertyFlagBits>(
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

        // for aspect flag
        VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        {
            // view
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

            switch (info.dimension)
            {
                case Dimension::e2D:
                    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    break;
                    // case TextureDimention::e3D:
                    //     ci.viewType = VK_IMAGE_VIEW_TYPE_3D;
                    //     break;
                default:
                    return Result::eFailure;
                    break;
            }

            ci.image      = io.mImage.value();
            ci.format     = io.format;
            ci.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A,
            };

            switch (info.usage)
            {
                case TextureUsage::eDepthStencilTarget:
                    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;// | VK_IMAGE_ASPECT_STENCIL_BIT;
                    ci.subresourceRange = io.range = {aspectFlag, 0, 1, 0, 1};
                    break;
                default:
                    ci.subresourceRange = io.range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                    break;
            }

            {
                VkImageView imageView;
                result =
                    checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &imageView));
                if (result != Result::eSuccess)
                {
                    std::cerr << "failed to create vkImageView!\n";
                    return result;
                }

                io.mView = imageView;
            }
        }

        {
            // TODO: impl changing sampler
            VkSamplerCreateInfo sci{};
            sci.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.pNext         = nullptr;
            sci.minFilter     = VK_FILTER_LINEAR;
            sci.magFilter     = VK_FILTER_LINEAR;
            sci.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.maxAnisotropy = 1.f;
            sci.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VkSampler sampler;
            result = checkVkResult(vkCreateSampler(mDevice, &sci, nullptr, &sampler));

            io.mSampler = sampler;
        }

        // set image layout
        {
            VkCommandBuffer command;
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandBufferCount = 1;
                ai.commandPool        = mCommandPool;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                vkAllocateCommandBuffers(mDevice, &ai, &command);
            }

            VkCommandBufferBeginInfo commandBI{};
            commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(command, &commandBI);

            setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_UNDEFINED,
                                  io.currentLayout, aspectFlag);
            vkEndCommandBuffer(command);

            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &command;
            vkQueueSubmit(mDeviceQueue, 1, &submitInfo, VK_NULL_HANDLE);

            // end copying
            vkDeviceWaitIdle(mDevice);
            vkFreeCommandBuffers(mDevice, mCommandPool, 1, &command);
        }

        handle_out = mNextTextureHandle++;
        mImageMap.emplace(handle_out, io);

        return Result::eSuccess;
    }

    Result Context::createTextureFromFile(const char* fileName,
                                          HTexture& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        ImageObject io;
        Result result;

        stbi_uc* pImage = nullptr;
        int width = 0, height = 0, channel = 0;
        io.format         = VK_FORMAT_R8G8B8A8_SRGB;  // fixed format
        io.mSizeOfChannel = 4;
        io.usage          = TextureUsage::eShaderResource;

        {
            VkImageCreateInfo ci{};

            if (!fileName)
            {
                std::cerr << "invalid file name\n";
                return Result::eFailure;
            }

            pImage  = stbi_load(fileName, &width, &height, nullptr, 4);
            channel = 4;
            if (!pImage)
            {
                std::cerr << "Failed to load texture file!\n";
                return Result::eFailure;
            }

            io.extent        = {uint32_t(width), uint32_t(height), 1};
            io.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // image
            ci.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.extent      = io.extent;
            ci.format      = io.format;
            ci.imageType   = VK_IMAGE_TYPE_2D;
            ci.arrayLayers = 1;
            ci.mipLevels   = 1;
            ci.samples     = VK_SAMPLE_COUNT_1_BIT;
            ci.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
        ai.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = reqs.size;
        // decide memory type
        VkMemoryPropertyFlagBits fb;
        fb = static_cast<VkMemoryPropertyFlagBits>(
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        io.mIsHostVisible  = true;
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
            // view
            VkImageViewCreateInfo ci{};
            ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.image    = io.mImage.value();
            ci.format   = io.format;

            ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            {
                VkImageView imageView;
                result =
                    checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &imageView));
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
            sci.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.pNext     = nullptr;
            sci.minFilter = VK_FILTER_LINEAR;
            sci.magFilter = VK_FILTER_LINEAR;

            sci.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.anisotropyEnable = VK_FALSE;
            sci.maxAnisotropy    = 16.f;
            sci.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            sci.compareOp        = VK_COMPARE_OP_ALWAYS;
            sci.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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

    Result Context::getTextureSize(const HTexture& handle, uint32_t& width_out,
                                   uint32_t& height_out, uint32_t& depth_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eFailure;

        if (mImageMap.count(handle) <= 0)
        {
            assert(!"invalid texture handle");
            return Result::eFailure;
        }

        auto&& extent = mImageMap[handle].extent;

        width_out  = extent.width;
        height_out = extent.height;
        depth_out  = extent.depth;

        return Result::eSuccess;
    }

    Result Context::writeTexture(const void* const pData, const HTexture& handle)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eFailure;

        if (mImageMap.count(handle) <= 0)
        {
            assert(!"invalid texture handle!");
            return Result::eFailure;
        }

        ImageObject& io = mImageMap[handle];

        BufferObject stagingBo;
        {
            size_t imageSize = io.extent.width * io.extent.height * io.extent.depth *
                               io.mSizeOfChannel;
            VkBufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ci.size  = imageSize;
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
                ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ai.allocationSize  = reqs.size;
                ai.memoryTypeIndex = getMemoryTypeIndex(
                    reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

                {
                    VkDeviceMemory memory;
                    vkAllocateMemory(mDevice, &ai, nullptr, &memory);
                    stagingBo.mMemory = memory;
                }

                vkBindBufferMemory(mDevice, stagingBo.mBuffer.value(),
                                   stagingBo.mMemory.value(), 0);
            }

            void* p;
            result = checkVkResult(vkMapMemory(mDevice, stagingBo.mMemory.value(), 0,
                                               VK_WHOLE_SIZE, 0, &p));
            if (Result::eSuccess != result)
                return result;

            memcpy(p, pData, imageSize);
            vkUnmapMemory(mDevice, stagingBo.mMemory.value());
        }

        VkBufferImageCopy copyRegion{};
        copyRegion.imageExtent      = {static_cast<uint32_t>(io.extent.width),
                                  static_cast<uint32_t>(io.extent.height),
                                  static_cast<uint32_t>(io.extent.depth)};
        copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        VkCommandBuffer command;
        {
            VkCommandBufferAllocateInfo ai{};
            ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandBufferCount = 1;
            ai.commandPool        = mCommandPool;
            ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            vkAllocateCommandBuffers(mDevice, &ai, &command);
        }

        VkCommandBufferBeginInfo commandBI{};
        commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(command, &commandBI);
        setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdCopyBufferToImage(command, stagingBo.mBuffer.value(), io.mImage.value(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        setImageMemoryBarrier(command, io.mImage.value(),
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkEndCommandBuffer(command);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &command;
        vkQueueSubmit(mDeviceQueue, 1, &submitInfo, VK_NULL_HANDLE);

        // end copying
        vkDeviceWaitIdle(mDevice);
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &command);

        // release staging buffer
        vkFreeMemory(mDevice, stagingBo.mMemory.value(), nullptr);
        vkDestroyBuffer(mDevice, stagingBo.mBuffer.value(), nullptr);

        return Result::eSuccess;
    }

    Result Context::setImageMemoryBarrier(VkCommandBuffer command, VkImage image,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          VkImageAspectFlags aspectFlags)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        VkImageMemoryBarrier imb{};
        imb.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.oldLayout           = oldLayout;
        imb.newLayout           = newLayout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.subresourceRange    = {aspectFlags, 0, 1, 0, 1};
        imb.image               = image;

        // final stage that write to resource in pipelines
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        // next state that write to resource in pipelines
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        switch (oldLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                imb.srcAccessMask = 0;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
        }

        switch (newLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstStage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dstStage          = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
        }

        vkCmdPipelineBarrier(command, srcStage, dstStage, 0,
                             0,  // memoryBarrierCount
                             nullptr,
                             0,  // bufferMemoryBarrierCount
                             nullptr,
                             1,  // imageMemoryBarrierCount
                             &imb);

        return Result::eSuccess;
    }

    Result Context::createDepthBuffer(WindowObject& wo)
    {
        Result result;
        ImageObject io;

        {
            VkImageCreateInfo ci{};
            ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ci.pNext         = nullptr;
            ci.imageType     = VK_IMAGE_TYPE_2D;
            ci.format        = VK_FORMAT_D32_SFLOAT;
            ci.extent.width  = wo.mSwapchainExtent.width;
            ci.extent.height = wo.mSwapchainExtent.height;
            ci.extent.depth  = 1;
            ci.mipLevels     = 1;
            ci.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ci.samples       = VK_SAMPLE_COUNT_1_BIT;
            ci.arrayLayers   = 1;

            io.format         = ci.format;
            io.extent         = ci.extent;
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
            ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize  = reqs.size;
            ai.memoryTypeIndex = getMemoryTypeIndex(
                reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            {
                VkDeviceMemory memory;
                result = checkVkResult(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                io.mMemory = memory;
            }

            result = checkVkResult(
                vkBindImageMemory(mDevice, io.mImage.value(), io.mMemory.value(), 0));
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        {
            VkImageViewCreateInfo ci{};
            ci.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image      = io.mImage.value();
            ci.viewType   = VK_IMAGE_VIEW_TYPE_2D;
            ci.format     = io.format;
            ci.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A,
            };
            //ci.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};
            ci.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

            VkImageView imageView;
            result =
                checkVkResult(vkCreateImageView(mDevice, &ci, nullptr, &imageView));
            if (Result::eSuccess != result)
            {
                std::cerr << "failed to create vkImageView!\n";
                return result;
            }

            io.mView = imageView;
        }

        io.currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        io.usage         = TextureUsage::eDepthStencilTarget;

        // set image layout
        {
            VkCommandBuffer command;
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandBufferCount = 1;
                ai.commandPool        = mCommandPool;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                vkAllocateCommandBuffers(mDevice, &ai, &command);
            }

            VkCommandBufferBeginInfo commandBI{};
            commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(command, &commandBI);

            //setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_UNDEFINED,
            //                      io.currentLayout, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_UNDEFINED,
                io.currentLayout, VK_IMAGE_ASPECT_DEPTH_BIT);
            vkEndCommandBuffer(command);

            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &command;
            vkQueueSubmit(mDeviceQueue, 1, &submitInfo, VK_NULL_HANDLE);

            // end copying
            vkDeviceWaitIdle(mDevice);
            vkFreeCommandBuffers(mDevice, mCommandPool, 1, &command);
        }

        mImageMap.emplace(mNextTextureHandle, io);
        wo.mHDepthBuffer = mNextTextureHandle++;

        return Result::eSuccess;
    }

    Result Context::setUpImGui(WindowObject& wo, RenderPassObject& rpo)
    {
        Result result = Result::eSuccess;

        // Create Descriptor Pool for ImGui
        {
            VkDescriptorPool descriptorPool;
            constexpr int maxNum              = 100;
            VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_SAMPLER, maxNum},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxNum},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxNum},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxNum},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, maxNum},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, maxNum},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxNum},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxNum},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxNum},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, maxNum},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxNum}};

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets                    = maxNum * IM_ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes                 = pool_sizes;
            result                               = checkVkResult(
                                              vkCreateDescriptorPool(mDevice, &pool_info, NULL, &descriptorPool));
            if (Result::eSuccess != result)
            {
                std::cerr << "failed to create vkDescriptorPool for ImGui!\n";
                return result;
            }
            mImGuiDescriptorPool = descriptorPool;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(wo.mpWindow.value(), true);

        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance                  = mInstance;
        info.PhysicalDevice            = mPhysDev;
        info.Device                    = mDevice;
        info.QueueFamily               = mGraphicsQueueIndex;
        info.Queue                     = mDeviceQueue;
        info.PipelineCache             = NULL;
        info.DescriptorPool            = mImGuiDescriptorPool.value();
        info.Allocator                 = NULL;
        info.MinImageCount             = std::max(2u, wo.mSurfaceCaps.minImageCount);
        info.ImageCount                = wo.mMaxFrameNum;
        info.CheckVkResultFn           = ImGui_check_vk_result;
        // ImGui_ImplVulkan_Init(&info, mImGuiRenderPass.value());
        ImGui_ImplVulkan_Init(&info, rpo.mRenderPass.value());

        {  // upload font data

            VkCommandBuffer commandBuffer;
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandPool        = mCommandPool;
                ai.commandBufferCount = 1;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                result =
                    checkVkResult(vkAllocateCommandBuffers(mDevice, &ai, &commandBuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to allocate command buffers!\n";
                    return result;
                }
            }

            VkCommandBufferBeginInfo bi = {};
            bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            result = checkVkResult(vkBeginCommandBuffer(commandBuffer, &bi));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to begin command buffer!\n";
                return result;
            }

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            VkSubmitInfo end_info       = {};
            end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers    = &commandBuffer;
            result                      = checkVkResult(vkEndCommandBuffer(commandBuffer));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to end command buffer!\n";
                return result;
            }

            result = checkVkResult(
                vkQueueSubmit(mDeviceQueue, 1, &end_info, VK_NULL_HANDLE));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to submit command buffer!\n";
                return result;
            }

            result = checkVkResult(vkDeviceWaitIdle(mDevice));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to wait device idle!\n";
                return result;
            }

            ImGui_ImplVulkan_DestroyFontUploadObjects();

            vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
        }

        return Result::eSuccess;
    }

    Result Context::uploadFontFile(const char* fontPath, float fontSize)
    {
        Result result = Result::eSuccess;

        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.Fonts->AddFontFromFileTTF(fontPath, fontSize);

        {  // upload font data

            VkCommandBuffer commandBuffer;
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandPool        = mCommandPool;
                ai.commandBufferCount = 1;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                result =
                    checkVkResult(vkAllocateCommandBuffers(mDevice, &ai, &commandBuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to allocate command buffers!\n";
                    return result;
                }
            }

            VkCommandBufferBeginInfo bi = {};
            bi.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            result = checkVkResult(vkBeginCommandBuffer(commandBuffer, &bi));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to begin command buffer!\n";
                return result;
            }

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            VkSubmitInfo end_info       = {};
            end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers    = &commandBuffer;
            result                      = checkVkResult(vkEndCommandBuffer(commandBuffer));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to end command buffer!\n";
                return result;
            }

            result = checkVkResult(
                vkQueueSubmit(mDeviceQueue, 1, &end_info, VK_NULL_HANDLE));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to submit command buffer!\n";
                return result;
            }

            result = checkVkResult(vkDeviceWaitIdle(mDevice));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to wait device idle!\n";
                return result;
            }

            ImGui_ImplVulkan_DestroyFontUploadObjects();

            vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
        }

        return result;
    }

    Result Context::createShaderModule(const Shader& shader,
                                       const VkShaderStageFlagBits& stage,
                                       VkPipelineShaderStageCreateInfo* pSSCI)
    {
        Result result;

        // std::ifstream infile(shader.path.c_str(), std::ios::binary);
        // if (!infile)
        // {
        //     std::cerr << "failed to load file from " << shader.path << " \n";
        //     return Result::eFailure;
        // }

        // std::vector<char> filedata;
        // filedata.resize(uint32_t(infile.seekg(0,
        // std::ifstream::end).tellg())); infile.seekg(0,
        // std::ifstream::beg).read(filedata.data(), filedata.size());

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.pNext = nullptr;
        ci.pCode =
            reinterpret_cast<const uint32_t*>(shader.getShaderByteCode().data());
        ci.codeSize = shader.getShaderByteCode().size();
        result =
            checkVkResult(vkCreateShaderModule(mDevice, &ci, nullptr, &shaderModule));
        if (result != Result::eSuccess)
        {
            std::cerr << "failed to create Shader Module\n";
            return result;
        }

        pSSCI->sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pSSCI->flags  = 0;
        pSSCI->pNext  = nullptr;
        pSSCI->stage  = stage;
        pSSCI->module = shaderModule;
        pSSCI->pName  = shader.getEntryPoint().data();

        return Result::eSuccess;
    }

    Result Context::createRenderPass(const RenderPassInfo& info,
                                     HRenderPass& handle_out)
    {
        if (info.window)
        {
            return createRenderPass(info.window.value(), true, handle_out);
        }
        else
        {
            if (info.depthTarget)
                return createRenderPass(info.colorTargets, info.depthTarget.value(),
                                        info.loadPrevFrame, handle_out);
            else
                return createRenderPass(info.colorTargets, info.loadPrevFrame,
                                        handle_out);
        }

        return Result::eFailure;
    }

    Result Context::createRenderPass(const HWindow& handle, bool depthTestEnable,
                                     HRenderPass& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result;
        RenderPassObject rpo;
        rpo.mFrameBufferIndex = 0;
        rpo.mDepthTestEnable  = depthTestEnable;
        rpo.mLoadPrevData     = false;

        // Renderpass, framebuffer

        if (mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid swapchain handle\n";
            return Result::eFailure;
        }
        rpo.mHWindow    = handle;
        auto& swapchain = mWindowMap[handle];

        {
            VkExtent3D extent;
            extent.width  = swapchain.mSwapchainExtent.width;
            extent.height = swapchain.mSwapchainExtent.height;
            extent.depth  = 1;
            rpo.mExtent   = extent;
        }

        {
            VkRenderPassCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            std::vector<VkAttachmentDescription> adVec;
            VkAttachmentReference ar;
            VkAttachmentReference depthAr;

            adVec.reserve(2);

            const auto& rdst = swapchain.mHSwapchainImages[0];  // only for info

            if (mImageMap.count(rdst) <= 0)
            {
                std::cerr << "invalid swapchain image handle\n";
                return Result::eFailure;
            }

            auto& io = mImageMap[rdst];
            adVec.emplace_back();
            adVec.back().format        = io.format;
            adVec.back().samples       = VK_SAMPLE_COUNT_1_BIT;
            adVec.back().loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
            adVec.back().storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
            adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            // adVec.back().finalLayout = swapchain.useImGui ?
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
            // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            adVec.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            ar.attachment            = 0;
            ar.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassDesc{};
            subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDesc.colorAttachmentCount = 1;
            subpassDesc.pColorAttachments    = &ar;

            VkSubpassDependency dependency{};
            dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass    = 0;
            dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            // if use depthBuffer, create attachment
            if (depthTestEnable)
            {
                rpo.depthTarget = swapchain.mHDepthBuffer;

                adVec.emplace_back();

                auto& depthBuffer          = mImageMap[swapchain.mHDepthBuffer];
                adVec.back().format        = depthBuffer.format;
                adVec.back().samples       = VK_SAMPLE_COUNT_1_BIT;
                adVec.back().loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
                adVec.back().storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
                adVec.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                adVec.back().finalLayout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthAr.attachment                  = 1;
                depthAr.layout                      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                subpassDesc.pDepthStencilAttachment = &depthAr;
            }
            else
            {
                subpassDesc.pDepthStencilAttachment = nullptr;
            }

            ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            ci.attachmentCount = static_cast<uint32_t>(adVec.size());
            ci.pAttachments    = adVec.data();
            ci.subpassCount    = 1;
            ci.pSubpasses      = &subpassDesc;
            ci.dependencyCount = 1;
            ci.pDependencies   = &dependency;
            {
                VkRenderPass renderPass;
                result =
                    checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
                if (Result::eSuccess != result)
                {
                    return result;
                }

                rpo.mRenderPass = renderPass;
            }
        }

        VkFramebufferCreateInfo fbci{};
        fbci.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = rpo.mRenderPass.value();
        fbci.width      = swapchain.mSwapchainExtent.width;
        fbci.height     = swapchain.mSwapchainExtent.height;
        fbci.layers     = 1;

        if (depthTestEnable)
        {
            const auto& depth    = mImageMap[swapchain.mHDepthBuffer];
            fbci.attachmentCount = 2;
            for (auto& h : swapchain.mHSwapchainImages)
            {
                const auto& img                  = mImageMap[h];
                std::array<VkImageView, 2> ivArr = {img.mView.value(),
                                                    depth.mView.value()};
                fbci.pAttachments                = ivArr.data();

                VkFramebuffer framebuffer;
                result = checkVkResult(
                    vkCreateFramebuffer(mDevice, &fbci, nullptr, &framebuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to create frame buffer!\n";
                    return result;
                }
                rpo.mFramebuffers.emplace_back(framebuffer);
            }
        }
        else
        {
            fbci.attachmentCount = 1;
            for (auto& h : swapchain.mHSwapchainImages)
            {
                fbci.pAttachments = &mImageMap[h].mView.value();

                VkFramebuffer frameBuffer;
                result = checkVkResult(
                    vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to create framebuffer!\n";
                    return result;
                }

                rpo.mFramebuffers.emplace_back(frameBuffer);
            }
        }

        createSyncObjects(rpo);

        // ImGuiを使用する場合はここで設定を行う
        if (swapchain.useImGui)
        {
            std::cerr << "set up ImGui\n";
            result = setUpImGui(swapchain, rpo);
            if (Result::eSuccess != result)
            {
                return result;
            }
        }

        handle_out = mNextRenderPassHandle++;
        mRPMap.emplace(handle_out, rpo);

        return Result::eSuccess;
    }

    Result Context::createRenderPass(const std::vector<HTexture>& colorTargets,
                                     const HTexture& depthTarget,
                                     const bool loadPrevFrame,
                                     HRenderPass& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result;
        RenderPassObject rpo;
        rpo.mFrameBufferIndex = 0;
        rpo.mDepthTestEnable  = true;
        rpo.mLoadPrevData     = loadPrevFrame;

        if (mDebugFlag)
        {  // for debug mode
            for (auto& tex : colorTargets)
            {
                if (mImageMap.count(tex) <= 0)
                {
                    std::cerr << "invalid texture handle\n";
                    return Result::eFailure;
                }
                auto& io = mImageMap[tex];

                // usage check
                if (io.usage != TextureUsage::eColorTarget &&
                    io.usage != TextureUsage::eUnordered)
                {
                    std::cerr << "invalid texture usage\n";
                    return Result::eFailure;
                }

                // extent substitute and check
                if (!rpo.mExtent)
                    rpo.mExtent = io.extent;
                if (rpo.mExtent.value().width != io.extent.width ||
                    rpo.mExtent.value().height != io.extent.height ||
                    rpo.mExtent.value().depth != io.extent.depth)
                {
                    std::cerr << "invalid color texture extent\n";
                    return Result::eFailure;
                }
            }

            if (mImageMap.count(depthTarget) <= 0)
            {
                std::cerr << "invalid texture handle\n";
                return Result::eFailure;
            }

            auto& io = mImageMap[depthTarget];
            if (io.usage != TextureUsage::eDepthStencilTarget)
            {
                std::cerr << "invalid texture usage!\n";
                return Result::eFailure;
            }

            // extent substitute and check
            if (!rpo.mExtent)
                rpo.mExtent = io.extent;
            if (rpo.mExtent.value().width != io.extent.width ||
                rpo.mExtent.value().height != io.extent.height ||
                rpo.mExtent.value().depth != io.extent.depth)
            {
                std::cerr << "invalid depth texture extent\n";
                std::cerr << rpo.mExtent.value().width << " : "
                          << rpo.mExtent.value().height << " : "
                          << rpo.mExtent.value().depth << "\n";
                std::cerr << io.extent.width << " : " << io.extent.height << " : "
                          << io.extent.depth << "\n";
                return Result::eFailure;
            }
        }

        rpo.mHWindow     = std::nullopt;
        rpo.colorTargets = colorTargets;
        rpo.depthTarget  = depthTarget;
        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        std::vector<VkAttachmentDescription> adVec;
        std::vector<VkAttachmentReference> arVec;

        // Renderpass, Framebuffer
        {
            auto&& size = colorTargets.size();
            for (size_t i = 0; i < size; ++i)
            {
                auto& io  = mImageMap[colorTargets[i]];
                auto&& ad = adVec.emplace_back(VkAttachmentDescription{});
                auto&& ar = arVec.emplace_back(VkAttachmentReference{});

                if (!rpo.mExtent)
                    rpo.mExtent = io.extent;

                if (loadPrevFrame)
                {
                    ad.loadOp  = VK_ATTACHMENT_LOAD_OP_LOAD;
                    ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                    switch (io.currentLayout)
                    {
                        case VK_IMAGE_LAYOUT_UNDEFINED:
                        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                            ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                            break;
                        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                            ad.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            break;
                        default:
                            std::cerr << "invalid initial image layout!\n";
                            return Result::eFailure;
                            break;
                    }
                }
                else
                {
                    ad.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    ad.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
                    ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }

                ad.format      = io.format;
                ad.samples     = VK_SAMPLE_COUNT_1_BIT;
                ad.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                ar.attachment  = i;
                ar.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
        }

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount = static_cast<uint32_t>(arVec.size());
        subpassDesc.pColorAttachments    = arVec.data();

        VkAttachmentReference depthAr;
        auto&& depthAd    = adVec.emplace_back();
        auto& depthBuffer = mImageMap[depthTarget];

        if (loadPrevFrame)
        {
            depthAd.loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAd.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            depthAd.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAd.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        depthAd.format      = depthBuffer.format;
        depthAd.samples     = VK_SAMPLE_COUNT_1_BIT;
        depthAd.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        depthAd.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAr.attachment  = colorTargets.size();  // attach to last index
        depthAr.layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpassDesc.pDepthStencilAttachment = &depthAr;

        ci.attachmentCount = static_cast<uint32_t>(adVec.size());
        ci.pAttachments    = adVec.data();
        ci.subpassCount    = 1;
        ci.pSubpasses      = &subpassDesc;

        {
            VkRenderPass renderPass;
            result =
                checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
            if (Result::eSuccess != result)
            {
                return result;
            }

            rpo.mRenderPass = renderPass;
        }

        std::vector<VkImageView> ivVec;
        ivVec.reserve(colorTargets.size());
        {
            for (auto& tex : colorTargets)
                ivVec.emplace_back(mImageMap[tex].mView.value());

            // depth buffer
            ivVec.emplace_back(depthBuffer.mView.value());
        }

        VkFramebufferCreateInfo fbci{};
        fbci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass      = rpo.mRenderPass.value();
        fbci.width           = rpo.mExtent.value().width;
        fbci.height          = rpo.mExtent.value().height;
        fbci.layers          = 1;
        fbci.attachmentCount = rpo.mTargetNum = static_cast<uint32_t>(ivVec.size());
        fbci.pAttachments                     = ivVec.data();

        {
            VkFramebuffer frameBuffer;
            result = checkVkResult(
                vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
            if (Result::eSuccess != result)
            {
                return result;
            }
            rpo.mFramebuffers.emplace_back(frameBuffer);
        }

        createSyncObjects(rpo);

        handle_out = mNextRenderPassHandle++;
        mRPMap.emplace(handle_out, rpo);

        return Result::eSuccess;
    }

    Result Context::createRenderPass(const std::vector<HTexture>& colorTargets,
                                     const bool loadPrevFrame,
                                     HRenderPass& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result;
        RenderPassObject rpo;
        rpo.mFrameBufferIndex = 0;
        rpo.mDepthTestEnable  = false;
        rpo.mLoadPrevData     = loadPrevFrame;

        if (mDebugFlag)
        {
            for (auto& tex : colorTargets)
            {
                if (mImageMap.count(tex) <= 0)
                {
                    std::cerr << "invalid texture handle!\n";
                    return Result::eFailure;
                }

                auto& io = mImageMap[tex];

                // usage check
                if (io.usage != TextureUsage::eColorTarget &&
                    io.usage != TextureUsage::eUnordered)
                {
                    std::cerr << "invalid texture usage!\n";
                    return Result::eFailure;
                }

                // extent substitute and check
                if (!rpo.mExtent)
                {
                    rpo.mExtent = io.extent;

                    if (rpo.mExtent.value().width != io.extent.width ||
                        rpo.mExtent.value().height != io.extent.height ||
                        rpo.mExtent.value().depth != io.extent.depth)
                    {
                        std::cerr << "invalid texture extent!\n";
                        return Result::eFailure;
                    }
                }
            }
        }

        rpo.mHWindow     = std::nullopt;
        rpo.colorTargets = colorTargets;

        VkRenderPassCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        std::vector<VkAttachmentDescription> adVec;
        std::vector<VkAttachmentReference> arVec;
        std::optional<HTexture> hDepthBuffer = std::nullopt;

        // Renderpass, Framebuffer
        {
            const auto&& size = colorTargets.size();

            adVec.reserve(size);
            arVec.reserve(size);
            for (size_t i = 0; i < size; ++i)
            {
                const auto& io = mImageMap[colorTargets[i]];
                auto&& ad      = adVec.emplace_back();
                auto&& ar      = arVec.emplace_back();

                if (!rpo.mExtent)
                    rpo.mExtent = io.extent;

                ad.format  = io.format;
                ad.samples = VK_SAMPLE_COUNT_1_BIT;

                if (loadPrevFrame)
                {
                    ad.loadOp  = VK_ATTACHMENT_LOAD_OP_LOAD;
                    ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                    switch (io.currentLayout)
                    {
                        case VK_IMAGE_LAYOUT_UNDEFINED:
                        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                            ad.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                            break;
                        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                            ad.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            break;
                        default:
                            std::cerr << "invalid initial image layout!\n";
                            return Result::eFailure;
                            break;
                    }
                }
                else
                {
                    ad.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    ad.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
                    ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }

                ad.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                ar.attachment  = i;
                ar.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
        }

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.colorAttachmentCount    = static_cast<uint32_t>(arVec.size());
        subpassDesc.pColorAttachments       = arVec.data();
        subpassDesc.pDepthStencilAttachment = NULL;

        rpo.mHWindow     = std::nullopt;
        rpo.colorTargets = colorTargets;

        std::array<VkSubpassDependency, 2> dependencies;
        {
            dependencies[0].srcSubpass   = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass   = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }

        ci.attachmentCount = static_cast<uint32_t>(adVec.size());
        ci.pAttachments    = adVec.data();
        ci.dependencyCount = 2;
        ci.pDependencies   = dependencies.data();
        ci.subpassCount    = 1;
        ci.pSubpasses      = &subpassDesc;

        {
            VkRenderPass renderPass;
            result =
                checkVkResult(vkCreateRenderPass(mDevice, &ci, nullptr, &renderPass));
            if (Result::eSuccess != result)
            {
                return result;
            }

            rpo.mRenderPass = renderPass;
        }

        std::vector<VkImageView> ivVec;
        {
            ivVec.reserve(colorTargets.size());
            for (const auto& tex : colorTargets)
                ivVec.emplace_back(mImageMap[tex].mView.value());
        }

        VkFramebufferCreateInfo fbci{};
        fbci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass      = rpo.mRenderPass.value();
        fbci.width           = rpo.mExtent.value().width;
        fbci.height          = rpo.mExtent.value().height;
        fbci.layers          = 1;
        fbci.attachmentCount = rpo.mTargetNum = static_cast<uint32_t>(ivVec.size());
        fbci.pAttachments                     = ivVec.data();

        {
            VkFramebuffer frameBuffer;
            result = checkVkResult(
                vkCreateFramebuffer(mDevice, &fbci, nullptr, &frameBuffer));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create frame buffer!\n";
                return result;
            }
            rpo.mFramebuffers.emplace_back(frameBuffer);
        }

        createSyncObjects(rpo);

        handle_out = mNextRenderPassHandle++;
        mRPMap.emplace(handle_out, rpo);

        return Result::eSuccess;
    }

    Result Context::createSyncObjects(RenderPassObject& rpo)
    {
        Result result = Result::eSuccess;

        uint32_t maxFramesInFlight = rpo.mTargetNum;
        uint32_t maxFrameNum       = rpo.mTargetNum;
        if (rpo.mHWindow)
        {
            auto& wo          = mWindowMap[rpo.mHWindow.value()];
            maxFramesInFlight = wo.mMaxFrameInFlight;
            maxFrameNum       = wo.mMaxFrameNum;
        }

        rpo.mFences.resize(maxFramesInFlight);
        {
            VkFenceCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            for (size_t i = 0; i < rpo.mFences.size(); ++i)
            {
                result =
                    checkVkResult(vkCreateFence(mDevice, &ci, nullptr, &rpo.mFences[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create fence!\n";
                    return result;
                }
            }
        }

        rpo.mPresentCompletedSems.resize(maxFramesInFlight);
        rpo.mRenderCompletedSems.resize(maxFramesInFlight);

        {
            VkSemaphoreCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            for (size_t i = 0; i < maxFramesInFlight; ++i)
            {
                result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr,
                                                         &rpo.mRenderCompletedSems[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create render completed semaphore!\n";
                    return result;
                }
            }

            for (size_t i = 0; i < maxFramesInFlight; ++i)
            {
                result = checkVkResult(vkCreateSemaphore(mDevice, &ci, nullptr,
                                                         &rpo.mPresentCompletedSems[i]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to create present completed semaphore!\n";
                    return result;
                }
            }
        }

        rpo.imagesInFlight.resize(maxFrameNum, VK_NULL_HANDLE);

        return result;
    }

    Result Context::createGraphicsPipeline(const GraphicsPipelineInfo& info,
                                           HGraphicsPipeline& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result;
        GraphicsPipelineObject gpo;

        RenderPassObject& rpo = mRPMap[info.renderPass];
        gpo.mHRenderPass      = info.renderPass;
        gpo.mVS               = info.VS;
        gpo.mFS               = info.FS;

        {
            VkVertexInputBindingDescription ib;

            std::vector<VkVertexInputAttributeDescription> ia_vec;

            VkPipelineVertexInputStateCreateInfo visci{};
            const auto& inputVariables = info.VS.getInputVariables();
            if (!inputVariables.empty())
            {
                {
                    ib.binding   = 0;
                    ib.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                }

                {
                    // if (inputVariables.size() == 0)
                    // {
                    //     std::cerr << "invalid vertex layout size!\n";
                    //     return Result::eFailure;
                    // }

                    uint32_t offset = 0;
                    for (size_t i = 0; i < inputVariables.size(); ++i)
                    {
                        ia_vec.emplace_back();
                        ia_vec.back().binding  = 0;
                        ia_vec.back().location = static_cast<uint32_t>(i);
                        ia_vec.back().offset   = offset;

                        switch (inputVariables[i].first)
                        {
                            case ResourceType::eUnorm:
                                ia_vec.back().format = VK_FORMAT_R8_UNORM;
                                offset += 1;
                                break;
                            case ResourceType::eFloat32:
                                ia_vec.back().format = VK_FORMAT_R32_SFLOAT;
                                offset += 4;
                                break;
                            case ResourceType::eUint32:
                                ia_vec.back().format = VK_FORMAT_R32_UINT;
                                offset += 4;
                                break;
                            case ResourceType::eInt32:
                                ia_vec.back().format = VK_FORMAT_R32_SINT;
                                offset += 4;
                                break;

                            case ResourceType::eUNormVec2:
                                ia_vec.back().format = VK_FORMAT_R8G8_UNORM;
                                offset += 2;
                                break;
                            case ResourceType::eF32Vec2:
                                ia_vec.back().format = VK_FORMAT_R32G32_SFLOAT;
                                offset += 8;
                                break;
                            case ResourceType::eU32Vec2:
                                ia_vec.back().format = VK_FORMAT_R32G32_UINT;
                                offset += 8;
                                break;
                            case ResourceType::eS32Vec2:
                                ia_vec.back().format = VK_FORMAT_R32G32_SINT;
                                offset += 8;
                                break;

                            case ResourceType::eUNormVec3:
                                ia_vec.back().format = VK_FORMAT_R8G8B8_UNORM;
                                offset += 3;
                                break;
                            case ResourceType::eF32Vec3:
                                ia_vec.back().format = VK_FORMAT_R32G32B32_SFLOAT;
                                offset += 12;
                                break;
                            case ResourceType::eU32Vec3:
                                ia_vec.back().format = VK_FORMAT_R32G32B32_UINT;
                                offset += 12;
                                break;
                            case ResourceType::eS32Vec3:
                                ia_vec.back().format = VK_FORMAT_R32G32B32_SINT;
                                offset += 12;
                                break;

                            case ResourceType::eUNormVec4:
                                ia_vec.back().format = VK_FORMAT_R8G8B8A8_UNORM;
                                offset += 4;
                                break;
                            case ResourceType::eF32Vec4:
                                ia_vec.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;
                                offset += 16;
                                break;
                            case ResourceType::eU32Vec4:
                                ia_vec.back().format = VK_FORMAT_R32G32B32A32_UINT;
                                offset += 16;
                                break;
                            case ResourceType::eS32Vec4:
                                ia_vec.back().format = VK_FORMAT_R32G32B32A32_SINT;
                                offset += 16;
                                break;
                            default:
                                std::cerr << "Vertex resource type (" << i
                                          << ") is not described\n";
                                return Result::eFailure;
                                break;
                        }
                    }

                    ib.stride = offset;
                }

                {
                    visci.sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                    visci.pNext                         = nullptr;
                    visci.vertexBindingDescriptionCount = 1;
                    visci.pVertexBindingDescriptions    = &ib;
                    visci.vertexAttributeDescriptionCount =
                        static_cast<uint32_t>(ia_vec.size());
                    visci.pVertexAttributeDescriptions = ia_vec.data();
                }
            }
            else  // vertex input state
            {
                visci.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                visci.pNext                           = nullptr;
                visci.vertexBindingDescriptionCount   = 0;
                visci.pVertexBindingDescriptions      = nullptr;
                visci.vertexAttributeDescriptionCount = 0;
                visci.pVertexAttributeDescriptions    = nullptr;
            }

            // std::cerr << "vertex input\n";

            // color blend
            std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
            blendAttachments.reserve(rpo.colorTargets.size() + 1);
            VkPipelineColorBlendStateCreateInfo cbci{};
            cbci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

            {
                auto&& size =
                    (rpo.mHWindow
                         ? 1
                         : rpo.colorTargets.size());  // + (rpo.depthTarget ? 1 : 0);
                for (size_t i = 0; i < size; ++i)
                {
                    auto&& blendAttachment = blendAttachments.emplace_back();

                    const auto colorWriteAll =
                        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                    switch (info.colorBlend)
                    {
                        case ColorBlend::eNone:
                            blendAttachment.blendEnable = VK_FALSE;
                            break;

                        case ColorBlend::eDefault:
                            blendAttachment.blendEnable         = VK_TRUE;
                            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                            blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
                            blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
                            blendAttachment.colorWriteMask      = colorWriteAll;
                            break;

                        case ColorBlend::eAlphaBlend:
                            blendAttachment.blendEnable         = VK_TRUE;
                            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                            blendAttachment.dstColorBlendFactor =
                                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                            blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
                            blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
                            blendAttachment.colorWriteMask      = colorWriteAll;
                            break;

                        default:
                            std::cerr << "color blend state is not described\n";
                            return Result::eFailure;
                            break;
                    }

                    cbci.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                    cbci.logicOpEnable   = VK_FALSE;
                    cbci.logicOp         = VK_LOGIC_OP_COPY;
                    cbci.attachmentCount = blendAttachments.size();
                    cbci.pAttachments    = blendAttachments.data();
                }
            }

            // viewport and scissor
            VkPipelineViewportStateCreateInfo vpsci{};
            VkViewport viewport{};
            VkRect2D scissor{};
            {
                if (info.viewport)
                {
                    viewport.x        = info.viewport.value()[0][0];
                    viewport.y        = info.viewport.value()[0][1];
                    viewport.minDepth = info.viewport.value()[0][2];

                    viewport.width    = info.viewport.value()[1][0];
                    viewport.height   = info.viewport.value()[1][1];
                    viewport.maxDepth = info.viewport.value()[1][2];
                }
                else
                {
                    const VkExtent3D& extent = rpo.mExtent.value();
                    viewport.x               = 0;
                    viewport.y               = 0;
                    viewport.width           = static_cast<float>(extent.width);
                    viewport.height          = static_cast<float>(extent.height);
                    viewport.minDepth        = 0;
                    viewport.maxDepth        = static_cast<float>(extent.depth);
                }

                if (info.scissor)
                {
                    scissor.offset.x = static_cast<int32_t>(info.scissor.value()[0][0]);
                    scissor.offset.y = static_cast<int32_t>(info.scissor.value()[0][1]);
                    scissor.extent   = {static_cast<uint32_t>(info.scissor.value()[1][0]),
                                      static_cast<uint32_t>(info.scissor.value()[1][1])};
                }
                else
                {
                    scissor.offset = {0, 0};
                    scissor.extent = {rpo.mExtent.value().width,
                                      rpo.mExtent.value().height};
                }

                vpsci.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                vpsci.viewportCount = 1;
                vpsci.pViewports    = &viewport;
                vpsci.scissorCount  = 1;
                vpsci.pScissors     = &scissor;
            }

            // std::cerr << "color and viewport\n";

            // input assembly
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
                    case Topology::eTriangleFan:
                        iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                        break;
                    default:
                        std::cerr << "primitive topology is not described\n";
                        return Result::eFailure;
                        break;
                }
            }

            // rasterization
            VkPipelineRasterizationStateCreateInfo rci{};
            {
                rci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                switch (info.rasterizerState.polygonMode)
                {
                    case PolygonMode::eFill:
                        rci.polygonMode = VK_POLYGON_MODE_FILL;
                        break;
                    case PolygonMode::eLine:
                        rci.polygonMode = VK_POLYGON_MODE_LINE;
                        break;
                    case PolygonMode::ePoint:
                        rci.polygonMode = VK_POLYGON_MODE_POINT;
                        break;
                    default:
                        std::cerr << "invalid polygon mode!\n";
                        return Result::eFailure;
                        break;
                }

                switch (info.rasterizerState.cullMode)
                {
                    case CullMode::eNone:
                        rci.cullMode = VK_CULL_MODE_NONE;
                        break;
                    case CullMode::eBack:
                        rci.cullMode = VK_CULL_MODE_BACK_BIT;
                        break;
                    case CullMode::eFront:
                        rci.cullMode = VK_CULL_MODE_FRONT_BIT;
                        break;
                    default:
                        std::cerr << "invalid culling mode!\n";
                        return Result::eFailure;
                        break;
                }

                switch (info.rasterizerState.frontFace)
                {
                    case FrontFace::eClockwise:
                        rci.frontFace = VK_FRONT_FACE_CLOCKWISE;
                        break;
                    case FrontFace::eCounterClockwise:
                        rci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                        break;
                    default:
                        std::cerr << "invalid front face mode!\n";
                        return Result::eFailure;
                        break;
                }

                rci.lineWidth = info.rasterizerState.lineWidth;
            }

            // multi sampling
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

            // std::cerr << "multi sample topology rasterizer state\n";

            // depth stencil
            VkPipelineDepthStencilStateCreateInfo dsci{};
            {
                dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                switch (info.depthStencilState)
                {
                    case DepthStencilState::eNone:
                        dsci.depthTestEnable = dsci.stencilTestEnable = VK_FALSE;
                        dsci.depthWriteEnable                         = VK_FALSE;
                        break;
                    case DepthStencilState::eDepth:
                        dsci.depthTestEnable       = VK_TRUE;
                        dsci.depthWriteEnable      = VK_TRUE;
                        dsci.stencilTestEnable     = VK_FALSE;
                        dsci.depthCompareOp        = VK_COMPARE_OP_LESS;
                        dsci.depthBoundsTestEnable = VK_FALSE;
                        break;
                    case DepthStencilState::eStencil:
                        dsci.depthTestEnable       = VK_FALSE;
                        dsci.depthWriteEnable      = VK_FALSE;
                        dsci.stencilTestEnable     = VK_TRUE;
                        dsci.depthCompareOp        = VK_COMPARE_OP_LESS;
                        dsci.depthBoundsTestEnable = VK_FALSE;
                        break;
                    case DepthStencilState::eBoth:
                        dsci.depthTestEnable = dsci.stencilTestEnable = VK_TRUE;
                        dsci.depthWriteEnable                         = VK_TRUE;
                        break;
                    default:
                        std::cerr << "depth stencil state is not described\n";
                        return Result::eFailure;
                        break;
                }
            }
            std::array<VkPipelineShaderStageCreateInfo, 2> ssciArr{};
            result =
                createShaderModule(info.VS, VK_SHADER_STAGE_VERTEX_BIT, &ssciArr[0]);
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create vertex shader module!\n";
                return Result::eFailure;
            }
            result =
                createShaderModule(info.FS, VK_SHADER_STAGE_FRAGMENT_BIT, &ssciArr[1]);
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to create fragment shader module!\n";
                return Result::eFailure;
            }

            auto layoutTable = info.VS.getLayoutTable();
            {  // integrate VS and FS layout
                auto fsLayoutTable = info.FS.getLayoutTable();
                layoutTable.merge(fsLayoutTable);
            }

            if (mDebugFlag)
            {
                for (const auto& p : layoutTable)
                {
                    std::cerr << "set: " << static_cast<int>(p.first.first)
                              << ", binding: " << static_cast<int>(p.first.second) << "\n";

                    switch (p.second)
                    {
                        case Shader::ShaderResourceType::eUniformBuffer:
                            std::cerr << " UniformBuffer\n";
                            break;
                        case Shader::ShaderResourceType::eCombinedTexture:
                            std::cerr << " CombinedTexture\n";
                            break;
                        case Shader::ShaderResourceType::eSampler:
                            std::cerr << " sampler\n";
                            break;
                    }
                }
            }

            uint32_t ubcount = 0;
            uint32_t ctcount = 0;
            {  // DescriptorSetLayout

                std::vector<std::vector<VkDescriptorSetLayoutBinding>> allBindings;
                allBindings.reserve(8);
                {  // HACK
                    uint32_t nowSet = UINT32_MAX;
                    for (const auto& [sb, srt] : layoutTable)
                    {
                        if (sb.first != nowSet)
                        {
                            allBindings.emplace_back();
                            nowSet = sb.first;
                        }

                        auto&& b          = allBindings.back().emplace_back();
                        b.binding         = sb.second;
                        b.descriptorCount = 1;
                        switch (srt)
                        {
                            case Shader::ShaderResourceType::eUniformBuffer:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                ++ubcount;
                                break;

                            case Shader::ShaderResourceType::eCombinedTexture:
                                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                                ++ctcount;
                                break;

                            case Shader::ShaderResourceType::eSampler:
                                assert(!"not supported");
                                break;
                        }

                        b.pImmutableSamplers = nullptr;
                        b.stageFlags         = VK_SHADER_STAGE_ALL;
                    }
                }

                VkDescriptorSetLayoutCreateInfo descLayoutci{};
                descLayoutci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                for (auto& bindings : allBindings)
                {
                    descLayoutci.bindingCount = static_cast<uint32_t>(bindings.size());
                    descLayoutci.pBindings    = bindings.data();

                    {
                        VkDescriptorSetLayout descriptorSetLayout;
                        result = checkVkResult(vkCreateDescriptorSetLayout(
                            mDevice, &descLayoutci, nullptr, &descriptorSetLayout));
                        if (result != Result::eSuccess)
                        {
                            std::cerr << "failed to create descriptor set layout\n";
                            return result;
                        }

                        gpo.mDescriptorSetLayouts.emplace_back(descriptorSetLayout);
                        gpo.mSetSizes.emplace_back(bindings.size());
                    }
                }
            }

            // if (layoutTable.size() > 0)
            //{ //DescriptorPool
            //    std::vector<VkDescriptorPoolSize> sizes;
            //    sizes.reserve(ubcount + ctcount);

            //    if(ubcount > 0)
            //    {
            //        sizes.emplace_back();
            //        sizes.back().descriptorCount = ubcount * mMaxFrame;//here
            //        sizes.back().type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            //    }

            //    if(ctcount > 0)
            //    {
            //        sizes.emplace_back();
            //        sizes.back().descriptorCount = ctcount * mMaxFrame;//here
            //        PART2 sizes.back().type =
            //        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            //    }
            //
            //    VkDescriptorPoolCreateInfo dpci{};
            //    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            //    dpci.flags =
            //    VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

            //    dpci.maxSets = layoutTable.size() * mMaxFrame;
            //
            //    dpci.poolSizeCount = static_cast<uint32_t>(sizes.size());
            //    dpci.pPoolSizes = sizes.data();
            //    {
            //        VkDescriptorPool descriptorPool;
            //        result = checkVkResult(vkCreateDescriptorPool(mDevice,
            //        &dpci, nullptr, &descriptorPool)); if (result !=
            //        Result::eSuccess)
            //        {
            //            std::cerr << "failed to create Descriptor Pool\n";
            //            return result;
            //        }
            //        gpo.mDescriptorPool = descriptorPool;
            //    }
            //}

            {  // pipeline layout
                VkPipelineLayoutCreateInfo ci{};
                ci.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                ci.setLayoutCount = gpo.mDescriptorSetLayouts.size();
                // VkDescriptorSetLayout dslayout =
                // gpo.mDescriptorSetLayout.value();
                ci.pSetLayouts = gpo.mDescriptorSetLayouts.data();

                {
                    VkPipelineLayout pipelineLayout;
                    result = checkVkResult(
                        vkCreatePipelineLayout(mDevice, &ci, nullptr, &pipelineLayout));
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "failed to create pipeline layout\n";
                        return result;
                    }

                    gpo.mPipelineLayout = pipelineLayout;
                }
            }

            {  // graphics pipeline layout
                VkGraphicsPipelineCreateInfo ci{};
                ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                ci.stageCount          = static_cast<uint32_t>(ssciArr.size());
                ci.pStages             = ssciArr.data();
                ci.pInputAssemblyState = &iaci;
                ci.pVertexInputState   = &visci;
                ci.pRasterizationState = &rci;
                ci.pDepthStencilState  = &dsci;
                ci.pMultisampleState   = &msci;
                ci.pViewportState      = &vpsci;
                ci.pColorBlendState    = &cbci;
                ci.renderPass          = rpo.mRenderPass.value();
                ci.layout              = gpo.mPipelineLayout.value();
                {
                    VkPipeline pipeline;
                    result = checkVkResult(vkCreateGraphicsPipelines(
                        mDevice, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline));
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "failed to create graphics pipeline\n";
                        return result;
                    }

                    gpo.mPipeline = pipeline;
                }
            }

            // won't be used
            for (auto& ssci : ssciArr)
                vkDestroyShaderModule(mDevice, ssci.module, nullptr);
        }

        handle_out = mNextGPHandle++;
        mGPMap.emplace(handle_out, gpo);

        return Result::eSuccess;
    }

    Result
    Context::createCommandBuffer(const std::vector<CommandList>& commandLists,
                                 HCommandBuffer& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eSuccess;

        CommandObject co;
        co.mPresentFlag = false;  // preset

        {  // allocate Descriptor from pool
            for (const auto& commandList : commandLists)
            {
                co.mUBCount += commandList.getUniformBufferCount();
                co.mCTCount += commandList.getCombinedTextureCount();
            }

            bool poolConfirmed = false;
            for (size_t i = 0; i < mDescriptorPools.size(); ++i)
                if (co.mUBCount + mDescriptorPools[i].first.uniformBufferCount <=
                        DescriptorPoolInfo::poolUBSize ||
                    co.mCTCount + mDescriptorPools[i].first.combinedTextureCount <=
                        DescriptorPoolInfo::poolCTSize)
                {
                    co.mDescriptorPoolIndex = i;
                    mDescriptorPools[i].first.uniformBufferCount += co.mUBCount;
                    mDescriptorPools[i].first.combinedTextureCount += co.mCTCount;
                    poolConfirmed = true;
                    break;
                }

            if (!poolConfirmed)
            {
                co.mDescriptorPoolIndex = mDescriptorPools.size();
                addDescriptorPool();
                mDescriptorPools.back().first.uniformBufferCount += co.mUBCount;
                mDescriptorPools.back().first.combinedTextureCount += co.mCTCount;
            }

            // resize descriptor sets
            co.mDescriptorSets.resize(commandLists.size());
        }

        uint32_t index = 0;
        co.mCommandBuffers.resize(commandLists.size());
        // get internal(public) command info vector
        for (const auto& commandList : commandLists)
        {
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandPool        = mCommandPool;
                ai.commandBufferCount = 1;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                result                = checkVkResult(
                                   vkAllocateCommandBuffers(mDevice, &ai, &co.mCommandBuffers[index]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to allocate command buffers!\n";
                    return result;
                }
            }

            const auto& cmdData = commandList.getInternalCommandData();

            {  // begin command buffer
                VkCommandBufferBeginInfo commandBI{};
                commandBI.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBI.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                commandBI.pInheritanceInfo = nullptr;

                result = checkVkResult(
                    vkBeginCommandBuffer(co.mCommandBuffers[index], &commandBI));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to begin command buffer!\n";
                    return result;
                }
            }

            writeCommandInternal(co, index, cmdData, commandList.useSubCommand());

            {  // end command buffer
                result = checkVkResult(vkEndCommandBuffer(co.mCommandBuffers[index]));

                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to end command buffer!\n";
                    return result;
                }
            }

            ++index;  // next command
        }

        handle_out = mNextCBHandle++;
        mCommandBufferMap.emplace(handle_out, co);

        return result;
    }

    Result Context::createCommandBuffer(const CommandList& commandList,
                                        HCommandBuffer& handle_out)
    {
        return createCommandBuffer(std::vector<CommandList>(1, commandList),
                                   handle_out);
    }

    Result Context::createSubCommandBuffer(const SubCommandList& subCommandList,
                                           HCommandBuffer& handle_out)
    {
        return createSubCommandBuffer(std::vector<SubCommandList>(1, subCommandList),
                                      handle_out);
    }

    Result Context::createSubCommandBuffer(
        const std::vector<SubCommandList>& subCommandLists,
        HCommandBuffer& handle_out)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eSuccess;

        CommandObject co;
        co.mPresentFlag = false;  // preset
        co.mSubCommand  = true;

        {  // allocate Descriptor from pool
            for (const auto& commandList : subCommandLists)
            {
                co.mUBCount += commandList.getUniformBufferCount();
                co.mCTCount += commandList.getCombinedTextureCount();
            }

            bool poolConfirmed = false;
            for (size_t i = 0; i < mDescriptorPools.size(); ++i)
                if (co.mUBCount + mDescriptorPools[i].first.uniformBufferCount <=
                        DescriptorPoolInfo::poolUBSize ||
                    co.mCTCount + mDescriptorPools[i].first.combinedTextureCount <=
                        DescriptorPoolInfo::poolCTSize)
                {
                    co.mDescriptorPoolIndex = i;
                    mDescriptorPools[i].first.uniformBufferCount += co.mUBCount;
                    mDescriptorPools[i].first.combinedTextureCount += co.mCTCount;
                    poolConfirmed = true;
                    break;
                }

            if (!poolConfirmed)
            {
                co.mDescriptorPoolIndex = mDescriptorPools.size();
                addDescriptorPool();
                mDescriptorPools.back().first.uniformBufferCount += co.mUBCount;
                mDescriptorPools.back().first.combinedTextureCount += co.mCTCount;
            }

            // resize descriptor sets
            co.mDescriptorSets.resize(subCommandLists.size());
        }

        uint32_t index = 0;
        co.mCommandBuffers.resize(subCommandLists.size());
        // get internal(public) command info vector
        for (const auto& subCommandList : subCommandLists)
        {
            {
                VkCommandBufferAllocateInfo ai{};
                ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.commandPool        = mCommandPool;
                ai.commandBufferCount = 1;
                ai.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                result                = checkVkResult(
                                   vkAllocateCommandBuffers(mDevice, &ai, &co.mCommandBuffers[index]));
                if (Result::eSuccess != result)
                {
                    std::cerr << "Failed to allocate command buffers!\n";
                    return result;
                }
            }

            const auto& cmdData = subCommandList.getInternalCommandData();

            {  // begin command buffer
                if (mDebugFlag && mRPMap.count(subCommandList.getRenderPass()) <= 0)
                {
                    std::cerr << "invalid render pass handle!\n";
                    return Result::eFailure;
                }
                auto& rpo = mRPMap[subCommandList.getRenderPass()];

                VkCommandBufferInheritanceInfo commandii{};
                commandii.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                commandii.pNext                = nullptr;
                commandii.renderPass           = rpo.mRenderPass.value();
                commandii.framebuffer          = rpo.mFramebuffers[index].value();
                commandii.subpass              = 0;
                commandii.occlusionQueryEnable = VK_FALSE;
                commandii.queryFlags           = 0;
                commandii.pipelineStatistics   = 0;

                VkCommandBufferBeginInfo commandBI{};
                commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                                  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                commandBI.pInheritanceInfo = &commandii;

                result = checkVkResult(
                    vkBeginCommandBuffer(co.mCommandBuffers[index], &commandBI));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to begin command buffer!\n";
                    return result;
                }
            }

            writeCommandInternal(co, index, cmdData);

            {  // end command buffer
                result = checkVkResult(vkEndCommandBuffer(co.mCommandBuffers[index]));

                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to end command buffer!\n";
                    return result;
                }
            }

            ++index;  // next command
        }

        handle_out = mNextCBHandle++;
        mCommandBufferMap.emplace(handle_out, co);

        return result;
    }

    Result Context::updateCommandBuffer(const std::vector<CommandList>& commandLists, const HCommandBuffer& handle)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eSuccess;

        if (mCommandBufferMap.count(handle) <= 0)
        {
            std::cerr << "create command buffer first!\n";
            return Result::eFailure;
        }

        CommandObject& co = mCommandBufferMap[handle];
        co.mPresentFlag   = false;

        uint32_t index = 0;
        if (co.mCommandBuffers.size() != commandLists.size())
        {
            std::cerr << "invalid rewriting commandlist size!\n";
            return Result::eFailure;
        }

        // for present command
        if (co.mHRenderPass && mRPMap.count(co.mHRenderPass.value()) > 0)
        {
            auto& rpo = mRPMap[co.mHRenderPass.value()];
            if (rpo.mHWindow)
            {
                auto& wo = mWindowMap[rpo.mHWindow.value()];
                for (size_t i = 0; i < wo.mMaxFrameInFlight; ++i)
                {
                    result = checkVkResult(
                        vkWaitForFences(mDevice, 1, &rpo.mFences[i], VK_TRUE, UINT64_MAX));
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "Failed to wait fence!\n";
                        return result;
                    }
                }
            }
            else
            {
                vkDeviceWaitIdle(mDevice);
            }
        }

        {  // free previous descriptor sets
            if (!co.mDescriptorSets.empty())
            {
                uint32_t time = 0;
                for (const auto& ds_vec : co.mDescriptorSets)
                {
                    std::vector<VkDescriptorSet> sets;
                    sets.reserve(ds_vec.size());
                    for (const auto& e : ds_vec)
                        sets.emplace_back(e.value());

                    if (!sets.empty())
                        vkFreeDescriptorSets(mDevice,
                                         mDescriptorPools[co.mDescriptorPoolIndex].second,
                                         sets.size(), sets.data());

                    auto& ubc = mDescriptorPools[co.mDescriptorPoolIndex].first.uniformBufferCount;
                    auto& ctc = mDescriptorPools[co.mDescriptorPoolIndex].first.combinedTextureCount;
                    ubc = (ubc >= co.mUBCount) ? ubc - co.mUBCount : 0;
                    ctc = (ctc >= co.mCTCount) ? ctc - co.mCTCount : 0;

                    co.mUBCount = 0;
                    co.mCTCount = 0;
                }

            }
        }

        {  // allocate Descriptor from pool
            for (const auto& commandList : commandLists)
            {
                co.mUBCount += commandList.getUniformBufferCount();
                co.mCTCount += commandList.getCombinedTextureCount();
            }

            bool poolConfirmed = false;
            for (size_t i = 0; i < mDescriptorPools.size(); ++i)
                if (co.mUBCount + mDescriptorPools[i].first.uniformBufferCount <=
                        DescriptorPoolInfo::poolUBSize &&
                    co.mCTCount + mDescriptorPools[i].first.combinedTextureCount <=
                        DescriptorPoolInfo::poolCTSize)
                {
                    co.mDescriptorPoolIndex = i;
                    mDescriptorPools[i].first.uniformBufferCount += co.mUBCount;
                    mDescriptorPools[i].first.combinedTextureCount += co.mCTCount;
                    poolConfirmed = true;
                    break;
                }

            if (!poolConfirmed)
            {
                co.mDescriptorPoolIndex = mDescriptorPools.size();
                addDescriptorPool();
                mDescriptorPools.back().first.uniformBufferCount += co.mUBCount;
                mDescriptorPools.back().first.combinedTextureCount += co.mCTCount;
            }
        }

        // clear barriered textures
        // co.mBarrieredTextures.clear();

        // get internal(public) command info vector
        for (const auto& commandList : commandLists)
        {
            const auto& cmdData = commandList.getInternalCommandData();

            vkResetCommandBuffer(co.mCommandBuffers[index], 0);

            {  // begin command buffer

                VkCommandBufferBeginInfo commandBI{};
                commandBI.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBI.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                commandBI.pInheritanceInfo = nullptr;

                result = checkVkResult(
                    vkBeginCommandBuffer(co.mCommandBuffers[index], &commandBI));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to begin command buffer!\n";
                    return result;
                }
            }

            writeCommandInternal(co, index, cmdData, commandList.useSubCommand());

            {  // end command buffer
                result = checkVkResult(vkEndCommandBuffer(co.mCommandBuffers[index]));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to end command buffer!\n";
                    return result;
                }
            }

            ++index;
        }

        // mCommandBufferMap.at(handle) = co;

        return result;
    }

    Result Context::updateCommandBuffer(const CommandList& commandList, const HCommandBuffer& handle)
    {
        return updateCommandBuffer(std::vector<CommandList>(1, commandList), handle);
    }

    Result Context::updateSubCommandBuffer(const SubCommandList& subCommandList, const HCommandBuffer& handle)
    {
        return updateSubCommandBuffer(std::vector<SubCommandList>(1, subCommandList), handle);
    }

    Result Context::updateSubCommandBuffer(const std::vector<SubCommandList>& subCommandLists, const HCommandBuffer& handle)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eSuccess;

        if (mCommandBufferMap.count(handle) <= 0)
        {
            std::cerr << "create command buffer first!\n";
            return Result::eFailure;
        }

        CommandObject& co = mCommandBufferMap[handle];
        co.mPresentFlag   = false;

        {  // free previous descriptor sets
            if (!co.mDescriptorSets.empty())
            {
                for (const auto& ds_vec : co.mDescriptorSets)
                {
                    std::vector<VkDescriptorSet> sets;
                    sets.reserve(ds_vec.size());
                    for (const auto& e : ds_vec)
                        sets.emplace_back(e.value());

                    if (!sets.empty())
                    vkFreeDescriptorSets(mDevice,
                                         mDescriptorPools[co.mDescriptorPoolIndex].second,
                                         sets.size(), sets.data());

                    auto& ubc =
                        mDescriptorPools[co.mDescriptorPoolIndex].first.uniformBufferCount;
                    auto& ctc = mDescriptorPools[co.mDescriptorPoolIndex]
                                    .first.combinedTextureCount;
                    ubc = ubc >= co.mUBCount ? ubc - co.mUBCount : 0;
                    ctc = ctc >= co.mCTCount ? ctc - co.mCTCount : 0;
                }
            }

            // co.mDescriptorSets.clear();
        }

        {  // allocate Descriptor from pool
            for (const auto& commandList : subCommandLists)
            {
                co.mUBCount += commandList.getUniformBufferCount();
                co.mCTCount += commandList.getCombinedTextureCount();
            }

            bool poolConfirmed = false;
            for (size_t i = 0; i < mDescriptorPools.size(); ++i)
                if (co.mUBCount + mDescriptorPools[i].first.uniformBufferCount <=
                        DescriptorPoolInfo::poolUBSize ||
                    co.mCTCount + mDescriptorPools[i].first.combinedTextureCount <=
                        DescriptorPoolInfo::poolCTSize)
                {
                    co.mDescriptorPoolIndex = i;
                    mDescriptorPools[i].first.uniformBufferCount += co.mUBCount;
                    mDescriptorPools[i].first.combinedTextureCount += co.mCTCount;
                    poolConfirmed = true;
                    break;
                }

            if (!poolConfirmed)
            {
                co.mDescriptorPoolIndex = mDescriptorPools.size();
                addDescriptorPool();
                mDescriptorPools.back().first.uniformBufferCount += co.mUBCount;
                mDescriptorPools.back().first.combinedTextureCount += co.mCTCount;
            }
        }

        // clear barriered textures
        // co.mBarrieredTextures.clear();

        if (co.mCommandBuffers.size() != subCommandLists.size())
        {
            std::cerr << "invalid rewriting commandlist size!\n";
            return Result::eFailure;
        }

        // for present command
        if (co.mHRenderPass && mRPMap.count(co.mHRenderPass.value()) > 0)
        {
            auto& rpo = mRPMap[co.mHRenderPass.value()];
            if (rpo.mHWindow)
            {
                auto& wo = mWindowMap[rpo.mHWindow.value()];
                for (size_t i = 0; i < wo.mMaxFrameInFlight; ++i)
                {
                    result = checkVkResult(
                        vkWaitForFences(mDevice, 1, &rpo.mFences[i], VK_TRUE, UINT64_MAX));
                    if (result != Result::eSuccess)
                    {
                        std::cerr << "Failed to wait fence!\n";
                        return result;
                    }
                }
            }
            else
            {
                vkDeviceWaitIdle(mDevice);
            }
        }

        uint32_t index = 0;
        // get internal(public) command info vector
        for (const auto& subCommandList : subCommandLists)
        {
            const auto& cmdData = subCommandList.getInternalCommandData();

            vkResetCommandBuffer(co.mCommandBuffers[index], 0);

            {  // begin command buffer
                auto& rpo = mRPMap[subCommandList.getRenderPass()];
                VkCommandBufferInheritanceInfo commandii{};
                commandii.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
                commandii.pNext                = nullptr;
                commandii.renderPass           = rpo.mRenderPass.value();
                commandii.framebuffer          = rpo.mFramebuffers[index].value();
                commandii.subpass              = 0;
                commandii.occlusionQueryEnable = VK_FALSE;
                commandii.queryFlags           = 0;
                commandii.pipelineStatistics   = 0;

                VkCommandBufferBeginInfo commandBI{};
                commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
                                  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                commandBI.pInheritanceInfo = &commandii;

                result = checkVkResult(
                    vkBeginCommandBuffer(co.mCommandBuffers[index], &commandBI));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to begin command buffer!\n";
                    return result;
                }
            }

            writeCommandInternal(co, index, cmdData);

            {  // end command buffer
                result = checkVkResult(vkEndCommandBuffer(co.mCommandBuffers[index]));
                if (result != Result::eSuccess)
                {
                    std::cerr << "Failed to end command buffer!\n";
                    return result;
                }
            }

            ++index;
        }

        // mCommandBufferMap.at(handle) = co;

        return result;
    }

    Result Context::writeCommandInternal(CommandObject& co, size_t index,
                                         const InternalCommandList& icl,
                                         const bool useSecondary)
    {
        Result result = Result::eSuccess;

        uint32_t debug = 0;

        for (const auto& command : icl)
        {
            // if (mDebugFlag)
            // std::cerr << "now command : " << debug++ << "\n";
            switch (command.first)  // RIP RTTI
            {
                case CommandType::eBegin:
                    if (!std::holds_alternative<CmdBegin>(command.second))
                        return Result::eFailure;
                    result = cmdBegin(co, index, std::get<CmdBegin>(command.second), useSecondary);
                    break;
                case CommandType::eEnd:
                    if (!std::holds_alternative<CmdEnd>(command.second))
                        return Result::eFailure;
                    result = cmdEnd(co, index, std::get<CmdEnd>(command.second));
                    break;
                case CommandType::eBindGraphicsPipeline:
                    if (!std::holds_alternative<CmdBindGraphicsPipeline>(command.second))
                        return Result::eFailure;
                    result = cmdBindGraphicsPipeline(
                        co, index, std::get<CmdBindGraphicsPipeline>(command.second));
                    break;
                case CommandType::ePresent:
                    if (!std::holds_alternative<CmdPresent>(command.second))
                        return Result::eFailure;
                    co.mPresentFlag = true;
                    break;
                case CommandType::eBindVB:
                    if (!std::holds_alternative<CmdBindVB>(command.second))
                        return Result::eFailure;
                    result = cmdBindVB(co, index, std::get<CmdBindVB>(command.second));
                    break;
                case CommandType::eBindIB:
                    if (!std::holds_alternative<CmdBindIB>(command.second))
                        return Result::eFailure;
                    result = cmdBindIB(co, index, std::get<CmdBindIB>(command.second));
                    break;
                case CommandType::eBindSRSet:
                    if (!std::holds_alternative<CmdBindSRSet>(command.second))
                        return Result::eFailure;
                    result = cmdBindSRSet(co, index, std::get<CmdBindSRSet>(command.second));
                    break;
                case CommandType::eRender:
                    if (!std::holds_alternative<CmdRender>(command.second))
                        return Result::eFailure;
                    result = cmdRender(co, index, std::get<CmdRender>(command.second));
                    break;
                case CommandType::eRenderIndexed:
                    if (!std::holds_alternative<CmdRenderIndexed>(command.second))
                        return Result::eFailure;
                    result = cmdRenderIndexed(co, index,
                                              std::get<CmdRenderIndexed>(command.second));
                    break;
                case CommandType::eBarrier:
                    if (!std::holds_alternative<CmdBarrier>(command.second))
                        return Result::eFailure;
                    result = cmdBarrier(co, index, std::get<CmdBarrier>(command.second));
                    break;
                case CommandType::eRenderImGui:
                    if (!std::holds_alternative<CmdRenderImGui>(command.second))
                        return Result::eFailure;
                    result = cmdRenderImGui(co, index);
                    break;
                case CommandType::eExecuteSubCommand:
                    if (!std::holds_alternative<CmdExecuteSubCommand>(command.second))
                        return Result::eFailure;
                    result = cmdExecuteSubCommand(
                        co, index, std::get<CmdExecuteSubCommand>(command.second));
                    break;
                default:
                    std::cerr << "invalid command!\nrequested command : "
                              << static_cast<int>(command.first) << "\n";
                    return Result::eFailure;
                    break;
            }

            if (result != Result::eSuccess)
                return result;
        }

        return Result::eSuccess;
    }

    Result Context::cmdBegin(CommandObject& co, size_t frameBufferIndex, const CmdBegin& info, const bool useSecondary)
    {
        Result result = Result::eSuccess;

        auto& command = co.mCommandBuffers[frameBufferIndex];
        if (mDebugFlag && mRPMap.count(info.handle) <= 0)
        {
            std::cerr << "invalid render pass!\n";
            return Result::eFailure;
        }

        auto& rpo = mRPMap[info.handle];

        co.mHRenderPass = info.handle;

        VkRenderPassBeginInfo bi{};

        std::vector<VkClearValue> clearValues;
        if (!rpo.mHWindow)
        {
            clearValues.reserve(rpo.colorTargets.size() + 1);

            for (size_t i = 0; i < rpo.colorTargets.size(); ++i)
            {
                clearValues.emplace_back();
                auto& cv = clearValues.back();
                auto& io = mImageMap[rpo.colorTargets[i]];
                switch (io.format)
                {
                    case VK_FORMAT_R32_UINT:
                    case VK_FORMAT_R32G32_UINT:
                    case VK_FORMAT_R32G32B32_UINT:
                    case VK_FORMAT_R32G32B32A32_UINT:
                        cv.color.uint32[0] = static_cast<uint32_t>(info.ccv[0] * 255);
                        cv.color.uint32[1] = static_cast<uint32_t>(info.ccv[1] * 255);
                        cv.color.uint32[2] = static_cast<uint32_t>(info.ccv[2] * 255);
                        cv.color.uint32[3] = static_cast<uint32_t>(info.ccv[3] * 255);
                        break;

                    case VK_FORMAT_R8_UNORM:
                    case VK_FORMAT_R8G8_UNORM:
                    case VK_FORMAT_R8G8B8_UNORM:
                    case VK_FORMAT_R8G8B8A8_UNORM:
                    case VK_FORMAT_R32_SFLOAT:
                    case VK_FORMAT_R32G32_SFLOAT:
                    case VK_FORMAT_R32G32B32_SFLOAT:
                    case VK_FORMAT_R32G32B32A32_SFLOAT:
                        cv.color.float32[0] = info.ccv[0];
                        cv.color.float32[1] = info.ccv[1];
                        cv.color.float32[2] = info.ccv[2];
                        cv.color.float32[3] = info.ccv[3];
                        break;

                    case VK_FORMAT_R32_SINT:
                    case VK_FORMAT_R32G32_SINT:
                    case VK_FORMAT_R32G32B32_SINT:
                    case VK_FORMAT_R32G32B32A32_SINT:
                        cv.color.int32[0] = static_cast<int32_t>(info.ccv[0] * 127);
                        cv.color.int32[1] = static_cast<int32_t>(info.ccv[1] * 127);
                        cv.color.int32[2] = static_cast<int32_t>(info.ccv[2] * 127);
                        cv.color.int32[3] = static_cast<int32_t>(info.ccv[3] * 127);
                        break;
                    default:
                        std::cerr << "invalid type of pixel!\n";
                        return Result::eFailure;
                        break;
                }
            }

            if (rpo.mDepthTestEnable)
            {
                clearValues.emplace_back().depthStencil = {std::get<0>(info.dcv),
                                                           std::get<1>(info.dcv)};
            }
        }
        else
        {
            clearValues.resize(2);
            clearValues[0].color        = {info.ccv[0], info.ccv[1], info.ccv[2], info.ccv[3]};
            clearValues[1].depthStencil = {std::get<0>(info.dcv),
                                           std::get<1>(info.dcv)};
        }

        bi.clearValueCount = clearValues.size();
        bi.pClearValues    = clearValues.data();

        if (!rpo.mHWindow && info.clear && !rpo.mLoadPrevData)
        {
            if (rpo.depthTarget)
            {
                auto& io = mImageMap[rpo.depthTarget.value()];
                if (io.currentLayout !=
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                {
                    // vkCmdClearDepthStencilImage(command, io.mImage.value(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    //                             &clearValues[1].depthStencil, 1, &io.range);

                    setImageMemoryBarrier(command, io.mImage.value(), io.currentLayout,
                                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                          io.range.aspectMask);
                    io.currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                // else
                //{
                //     setImageMemoryBarrier(command, io.mImage.value(),
                //     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                //     io.range.aspectMask);
                //     vkCmdClearDepthStencilImage(command, io.mImage.value(),
                //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                //     &clearValues[1].depthStencil, 1, &io.range);
                //     setImageMemoryBarrier(command, io.mImage.value(),
                //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                //     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                //     io.range.aspectMask); io.currentLayout =
                //     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                // }
            }

            for (const auto& tex : rpo.colorTargets)
            {
                auto& io = mImageMap[tex];
                // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL

                // setImageMemoryBarrier(command, io.mImage.value(),
                // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                // vkCmdClearColorImage(command, io.mImage.value(),
                //                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                //                      &clearValues[0].color, 1, &io.range);
                // setImageMemoryBarrier(command,
                //  io.mImage.value(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                //  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                if (io.currentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                {
                    setImageMemoryBarrier(command, io.mImage.value(), io.currentLayout,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                          io.range.aspectMask);
                    io.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
        }
        else if (info.clear && !rpo.mLoadPrevData)
        {
            for (const auto& tex : rpo.colorTargets)
            {
                auto& io = mImageMap[tex];
                if (io.currentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                {
                    setImageMemoryBarrier(
                        command, io.mImage.value(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, io.range.aspectMask);
                    io.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
        }
        else
        {
            if (rpo.depthTarget)
            {
                auto& io = mImageMap[rpo.depthTarget.value()];
                if (io.currentLayout !=
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                {
                    setImageMemoryBarrier(command, io.mImage.value(), io.currentLayout,
                                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                          io.range.aspectMask);
                    io.currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
            }

            for (const auto& img : rpo.colorTargets)
            {
                auto& io = mImageMap[img];
                if (io.currentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                {
                    setImageMemoryBarrier(command, io.mImage.value(), io.currentLayout,
                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                    io.currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
        }

        bi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        bi.renderPass        = rpo.mRenderPass.value();
        bi.renderArea.offset = {0, 0};
        bi.renderArea.extent = {rpo.mExtent.value().width,
                                rpo.mExtent.value().height};
        bi.framebuffer =
            rpo.mFramebuffers[frameBufferIndex % rpo.mFramebuffers.size()].value();

        if (useSecondary)
            vkCmdBeginRenderPass(command, &bi,
                                 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        else
            vkCmdBeginRenderPass(command, &bi, VK_SUBPASS_CONTENTS_INLINE);

        return Result::eSuccess;
    }

    Result Context::cmdEnd(CommandObject& co, size_t index, const CmdEnd& info)
    {
        auto& command = co.mCommandBuffers[index];
        auto& rpo     = mRPMap[co.mHRenderPass.value()];

        if (rpo.mHWindow)
        {
            for (auto& htex : rpo.colorTargets)
            {
                auto& io = mImageMap[htex];
                setImageMemoryBarrier(command, io.mImage.value(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            }
        }

        vkCmdEndRenderPass(command);

        return Result::eSuccess;
    }

    Result Context::cmdBindVB(CommandObject& co, size_t index,
                              const CmdBindVB& info)
    {
        if (mBufferMap.count(info.VBHandle) <= 0)
            return Result::eFailure;
        auto& vbo = mBufferMap[info.VBHandle];

        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(co.mCommandBuffers[index], 0, 1, &vbo.mBuffer.value(),
                               offsets);

        return Result::eSuccess;
    }

    Result Context::cmdBindIB(CommandObject& co, size_t index,
                              const CmdBindIB& info)
    {
        if (mBufferMap.count(info.IBHandle) <= 0)
            return Result::eFailure;
        auto& ibo = mBufferMap[info.IBHandle];

        vkCmdBindIndexBuffer(co.mCommandBuffers[index], ibo.mBuffer.value(), 0,
                             VK_INDEX_TYPE_UINT32);

        return Result::eSuccess;
    }

    Result Context::cmdBindGraphicsPipeline(CommandObject& co, size_t index,
                                            const CmdBindGraphicsPipeline& info)
    {
        auto& gpo     = mGPMap[info.handle];
        co.mHGPO      = info.handle;
        auto& command = co.mCommandBuffers[index];
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          gpo.mPipeline.value());

        // allocate descriptor sets
        co.mDescriptorSets[index].resize(gpo.mDescriptorSetLayouts.size());

        return Result::eSuccess;
    }

    Result Context::cmdBindSRSet(CommandObject& co, size_t index,
                                 const CmdBindSRSet& info)
    {
        Result result = Result::eSuccess;

        if (!co.mHGPO)
        {
            std::cerr << "graphics pipeline object is not registered yet!\n";
            return Result::eFailure;
        }

        auto& gpo = mGPMap[co.mHGPO.value()];

        if (mDescriptorPools.size() <= co.mDescriptorPoolIndex)
        {
            std::cerr << "descriptor pool is nothing!\n";
            return Result::eFailure;
        }

        auto&& UBs = info.SRSet.getUniformBuffers();
        auto&& CTs = info.SRSet.getCombinedTextures();

        std::vector<VkDescriptorBufferInfo> dbi_vec;
        dbi_vec.reserve(UBs.size());
        std::vector<VkDescriptorImageInfo> dii_vec;
        dii_vec.reserve(CTs.size());
        std::vector<VkWriteDescriptorSet> writeDescriptors;
        {
            VkDescriptorSetAllocateInfo dsai{};
            dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsai.descriptorPool     = mDescriptorPools[co.mDescriptorPoolIndex].second;
            dsai.descriptorSetCount = 1;
            dsai.pSetLayouts        = &gpo.mDescriptorSetLayouts[info.set];

            {
                VkDescriptorSet set;
                result = checkVkResult(vkAllocateDescriptorSets(mDevice, &dsai, &set));
                if (Result::eSuccess != result)
                {
                    std::cerr << "failed to allocate VkDescriptorSet!\n";
                    return Result::eFailure;
                }

                co.mDescriptorSets[index][info.set] = set;
            }

            if (result != Result::eSuccess)
            {
                std::cerr << "failed to allocate descriptor set\n";
                return result;
            }

            writeDescriptors.reserve(gpo.mSetSizes[info.set]);

            for (const auto& dub : UBs)
            {
                auto& ubo  = mBufferMap[dub.second];
                auto&& dbi = dbi_vec.emplace_back();
                dbi.buffer = ubo.mBuffer.value();
                dbi.offset = 0;
                dbi.range  = VK_WHOLE_SIZE;

                auto&& wdi     = writeDescriptors.emplace_back(VkWriteDescriptorSet{});
                wdi.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                wdi.dstBinding = dub.first;
                //if (mDebugFlag)
                //    std::cerr << "buffer dstBinding : " << wdi.dstBinding << "\n";
                wdi.dstArrayElement = 0;
                wdi.descriptorCount = 1;
                wdi.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                wdi.pBufferInfo     = &dbi_vec.back();
                wdi.dstSet          = co.mDescriptorSets[index][info.set].value();
            }

            for (const auto& dct : CTs)
            {
                auto& cto = mImageMap[dct.second];

                auto&& dii    = dii_vec.emplace_back();
                dii.imageView = cto.mView.value();
                dii.sampler   = cto.mSampler.value();
                // dii.imageLayout = cto.currentLayout;
                dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                // std::cerr << static_cast<int>(cto.currentLayout) << "\n";

                auto&& wdi     = writeDescriptors.emplace_back(VkWriteDescriptorSet{});
                wdi.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                wdi.dstBinding = dct.first;
                //if (mDebugFlag)
                //    std::cerr << "image dstBinding : " << wdi.dstBinding << "\n";
                wdi.dstArrayElement = 0;
                wdi.descriptorCount = 1;
                wdi.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                wdi.pImageInfo      = &dii_vec.back();
                wdi.dstSet          = co.mDescriptorSets[index][info.set].value();
            }

            vkUpdateDescriptorSets(mDevice,
                                   static_cast<uint32_t>(writeDescriptors.size()),
                                   writeDescriptors.data(), 0, nullptr);
        }

        return Result::eSuccess;
    }

    Result Context::cmdRenderIndexed(CommandObject& co, size_t index,
                                     const CmdRenderIndexed& info)
    {
        auto& gpo = mGPMap[co.mHGPO.value()];
        std::vector<VkDescriptorSet> sets;
        sets.reserve(co.mDescriptorSets[index].size());
        for (const auto& e : co.mDescriptorSets[index])
        {
            sets.emplace_back(e.value());
        }

        vkCmdBindDescriptorSets(
            co.mCommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS,
            gpo.mPipelineLayout.value(), 0, sets.size(), sets.data(), 0, nullptr);

        vkCmdDrawIndexed(co.mCommandBuffers[index], info.indexCount,
                         info.instanceCount, info.firstIndex, info.vertexOffset,
                         info.firstInstance);

        return Result::eSuccess;
    }

    Result Context::cmdRender(CommandObject& co, size_t index,
                              const CmdRender& info)
    {
        auto& gpo = mGPMap[co.mHGPO.value()];

        std::vector<VkDescriptorSet> sets;
        sets.reserve(co.mDescriptorSets[index].size());
        for (const auto& e : co.mDescriptorSets[index])
        {
            sets.emplace_back(e.value());
        }

        vkCmdBindDescriptorSets(
            co.mCommandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS,
            gpo.mPipelineLayout.value(), 0, sets.size(), sets.data(), 0, nullptr);

        vkCmdDraw(co.mCommandBuffers[index], info.vertexCount, info.instanceCount,
                  info.vertexOffset, info.firstInstance);

        return Result::eSuccess;
    }

    Result Context::cmdBarrier(CommandObject& co, size_t index,
                               const CmdBarrier& info)
    {
        if (mDebugFlag && mImageMap.count(info.handle) <= 0)
        {
            std::cerr << "invalid texture handle!\n";
            return Result::eFailure;
        }

        auto& io = mImageMap[info.handle];
        setImageMemoryBarrier(co.mCommandBuffers[index], io.mImage.value(),
                              io.currentLayout,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        io.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // co.mBarrieredTextures.emplace_back(info.handle);

        return Result::eSuccess;
    }

    Result Context::cmdExecuteSubCommand(CommandObject& co, size_t frameBufferIndex,
                                         const CmdExecuteSubCommand& info)
    {
        if (mDebugFlag && mCommandBufferMap.count(info.handle) <= 0)
        {
            std::cerr << "invalid sub command buffer handle!\n";
            return Result::eFailure;
        }

        auto&& subCO = mCommandBufferMap[info.handle];
        if (mDebugFlag && !subCO.mSubCommand)
        {
            std::cerr << "thi command buffer is primary!\n";
            return Result::eFailure;
        }

        vkCmdExecuteCommands(co.mCommandBuffers[frameBufferIndex], 1,
                             &subCO.mCommandBuffers[frameBufferIndex]);

        return Result::eSuccess;
    }

    uint32_t Context::getFrameBufferIndex(const HRenderPass& handle) const
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return 0;
        }

        if (mRPMap.count(handle) <= 0)
        {
            std::cerr << "invalid renderpass handle!\n";
            return 0;
        }

        return mRPMap.at(handle).mFrameBufferIndex;
    }

    Result Context::cmdRenderImGui(CommandObject& co, size_t frameBufferIndex)
    {
        Result result = Result::eSuccess;

        // Record dear imgui primitives into command buffer
        auto* pData = ImGui::GetDrawData();
        if (!pData)
        {
            std::cerr << "invalid ImGui command call!\n";
            return Result::eFailure;
        }

        ImGui_ImplVulkan_RenderDrawData(pData, co.mCommandBuffers[frameBufferIndex]);

        return result;
    }

    Result Context::execute(const HCommandBuffer& handle)
    {
        if (!mIsInitialized)
        {
            std::cerr << "context did not initialize yet!\n";
            return Result::eFailure;
        }

        Result result = Result::eSuccess;

        if (mDebugFlag && mCommandBufferMap.count(handle) <= 0)
        {
            std::cerr << "invalid commandbuffer handle!\n";
            return Result::eFailure;
        }

        auto& co = mCommandBufferMap[handle];
        if (!co.mHRenderPass)
        {
            std::cerr << "render pass of this command is invalid!\n";
            return Result::eFailure;
        }

        if (co.mSubCommand)
        {
            std::cerr << "this command buffer is sub(secondary)!\n";
            return Result::eFailure;
        }

        // control transition of texture layout by barrier
        //{
        //    for (auto& handle : co.mBarrieredTextures)
        //        mImageMap[handle].currentLayout =
        //        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //}

        // vkQueueWaitIdle(mDeviceQueue);
        // vkDeviceWaitIdle(mDevice);

        auto& rpo = mRPMap[co.mHRenderPass.value()];

        if (rpo.mHWindow && co.mPresentFlag)
        {
            auto& wo = mWindowMap[rpo.mHWindow.value()];

            result = checkVkResult(vkWaitForFences(
                mDevice, 1, &rpo.mFences[wo.mCurrentFrame], VK_TRUE, UINT64_MAX));
            if (result != Result::eSuccess)
            {
                std::cerr << "Failed to wait fence!\n";
                return result;
            }

            result = checkVkResult(
                vkAcquireNextImageKHR(mDevice, wo.mSwapchain.value(), UINT64_MAX,
                                      rpo.mPresentCompletedSems[wo.mCurrentFrame],
                                      VK_NULL_HANDLE, &rpo.mFrameBufferIndex));

            if (result != Result::eSuccess)
            {
                std::cerr << "failed to acquire next swapchain image!\n";
                return result;
            }

            if (rpo.imagesInFlight[rpo.mFrameBufferIndex] != VK_NULL_HANDLE)
                vkWaitForFences(mDevice, 1, &rpo.imagesInFlight[rpo.mFrameBufferIndex],
                                VK_TRUE, UINT64_MAX);

            rpo.imagesInFlight[rpo.mFrameBufferIndex] = rpo.mFences[wo.mCurrentFrame];

            // submit command

            VkSubmitInfo submitInfo{};
            VkPipelineStageFlags waitStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers =
                &co.mCommandBuffers[rpo.mFrameBufferIndex % co.mCommandBuffers.size()];
            submitInfo.pWaitDstStageMask    = &waitStageMask;
            submitInfo.waitSemaphoreCount   = 1;
            submitInfo.pWaitSemaphores      = &rpo.mPresentCompletedSems[wo.mCurrentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores    = &rpo.mRenderCompletedSems[wo.mCurrentFrame];
            result                          = checkVkResult(
                                         vkResetFences(mDevice, 1, &rpo.mFences[wo.mCurrentFrame]));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to reset fence!\n";
                return result;
            }

            result = checkVkResult(vkQueueSubmit(mDeviceQueue, 1, &submitInfo,
                                                 rpo.mFences[wo.mCurrentFrame]));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to submit cmd to queue!\n";
                return result;
            }

            // present
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.swapchainCount     = 1;
            presentInfo.pSwapchains        = &wo.mSwapchain.value();
            presentInfo.pImageIndices      = &rpo.mFrameBufferIndex;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores    = &rpo.mRenderCompletedSems[wo.mCurrentFrame];

            result = checkVkResult(vkQueuePresentKHR(mDeviceQueue, &presentInfo));
            if (Result::eSuccess != result)
            {
                std::cerr << "Failed to present queue!\n";
                return result;
            }

            wo.mCurrentFrame = (wo.mCurrentFrame + 1) % wo.mMaxFrameInFlight;
        }
        else
        {
            result = checkVkResult(
                vkWaitForFences(mDevice, 1, &rpo.mFences[0], VK_TRUE, UINT64_MAX));
            if (result != Result::eSuccess)
            {
                std::cerr << "Failed to wait fence!\n";
                return result;
            }

            result = checkVkResult(vkResetFences(mDevice, 1, &rpo.mFences[0]));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to reset fence!\n";
                return result;
            }

            // Maybe it will be useless
            rpo.mFrameBufferIndex =
                (rpo.mFrameBufferIndex + 1) % rpo.mFramebuffers.size();

            // submit command
            VkSubmitInfo submitInfo{};
            VkPipelineStageFlags waitStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &(
                co.mCommandBuffers[rpo.mFrameBufferIndex % co.mCommandBuffers.size()]);
            submitInfo.pWaitDstStageMask    = &waitStageMask;
            submitInfo.waitSemaphoreCount   = 0;
            submitInfo.pWaitSemaphores      = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.pSignalSemaphores    = nullptr;

            result = checkVkResult(vkQueueSubmit(mDeviceQueue, 1, &submitInfo,
                                                 rpo.mFences[rpo.mFrameBufferIndex]));
            if (result != Result::eSuccess)
            {
                std::cerr << "failed to submit cmd to queue!\n";
                return result;
            }
        }

        return Result::eSuccess;
    }

    // I/O-----------------------------------

    Result Context::updateInput() const
    {
        glfwPollEvents();
        return Result::eSuccess;
    }

    bool Context::getKey(const Key& key) const
    {
        static bool rtn = false;

        for (const auto& wo : mWindowMap)
        {
            // continue if hided
            if (!glfwGetWindowAttrib(wo.second.mpWindow.value(), GLFW_VISIBLE))
                continue;

            if (rtn = glfwGetKey(wo.second.mpWindow.value(), static_cast<int>(key)) > 0)
                return rtn;
        }

        return false;
    }

    bool Context::getKey(const HWindow& handle, const Key& key) const
    {
        if (mDebugFlag && mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid window handle!\n";
            return 0;
        }

        const auto& wo = mWindowMap.at(handle);
        return glfwGetKey(wo.mpWindow.value(), static_cast<int>(key)) == GLFW_PRESS;
    }

    Result Context::getMousePos(double& x, double& y) const
    {
        for (const auto& wo : mWindowMap)
        {
            // continue if not focused
            if (!glfwGetWindowAttrib(wo.second.mpWindow.value(), GLFW_FOCUSED))
                continue;

            glfwGetCursorPos(wo.second.mpWindow.value(), &x, &y);
            return Result::eSuccess;
        }

        // all windows were not focused
        x = 0;
        y = 0;
        return Result::eFailure;
    }

    Result Context::getMousePos(const HWindow& handle, double& x, double& y) const
    {
        if (mDebugFlag && mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid window handle!\n";
            return Result::eFailure;
        }

        const auto& wo = mWindowMap.at(handle);
        glfwGetCursorPos(wo.mpWindow.value(), &x, &y);
        return Result::eSuccess;
    }

    bool Context::shouldClose() const
    {
        for (const auto& wo : mWindowMap)
            if (glfwWindowShouldClose(wo.second.mpWindow.value()) == GL_TRUE)
                return true;

        return false;
    }

    bool Context::shouldClose(const HWindow& handle) const
    {
        if (mDebugFlag && mWindowMap.count(handle) <= 0)
        {
            std::cerr << "invalid window handle!\n";
            return false;
        }

        const auto& wo = mWindowMap.at(handle);
        return glfwWindowShouldClose(wo.mpWindow.value()) == GL_TRUE;
    }

};  // namespace Cutlass