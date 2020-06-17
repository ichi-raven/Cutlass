#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.hpp>
#include<vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <queue>


namespace Cutlass
{
    using HBuffer = uint64_t;
    using HTexture = uint64_t;
    using HSampler = uint64_t;
    using HRenderPipeline = uint64_t;

    enum class ResourceType
    {
        eInt,
        eUint,
        eVec2,
        eVec3,
        eVec4,
        eMat2,
        eMat3,
        eMat4,
    };

    enum class BufferUsage
    {
        eVertex,
        eIndex,
        eUniform,
    };

    struct VertexLayout
    {
        std::queue<ResourceType> layouts;
        std::queue<std::string> names;
    };

    struct ShaderResourceLayout
    {
        std::queue<HBuffer> constantBuffers;
        std::queue<HTexture> Textures;
    };

    class Device
    {
    public:
        Device();

        bool init();

        bool createBuffer(BufferUsage bufferUsage, HBuffer* hBuffer);

        bool createTexture(HTexture* hTexture);

        bool createSampler(HSampler *hSampler);

        bool createRenderPipeline(HRenderPipeline* hRenderPipeline);

        bool writeCommand();

        bool submitCommand();

        bool present();

    private: 
    
        struct BufferObject
        {
            VkBuffer buffer;
            VkDeviceMemory memory;
        };

        struct ImageObject
        {
            VkImage image;
            VkDeviceMemory memory;
            VkImageView view;
        };

        static void checkResult(VkResult);
        void initializeInstance(const char* appName);

        void selectPhysicalDevice();
        uint32_t searchGraphicsQueueIndex();
        void createDevice();
        void prepareCommandPool();
        void selectSurfaceFormat(VkFormat format);
        void createSwapchain(GLFWwindow* window);
        void createDepthBuffer();
        void createViews();

        void createRenderPass();
        void createFramebuffer();

        void prepareCommandBuffers();
        void prepareSemaphores();

        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps)const;

        void enableDebugReport();
        void disableDebugReport();

        GLFWwindow* mWindow;

        VkInstance  mInstance;
        VkDevice    mDevice;
        VkPhysicalDevice  mPhysDev;
        
        VkSurfaceKHR        mSurface;
        VkSurfaceFormatKHR  mSurfaceFormat;
        VkSurfaceCapabilitiesKHR  mSurfaceCaps;
        
        VkPhysicalDeviceMemoryProperties mPhysMemProps;
        
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        
        VkCommandPool mCommandPool;
        VkPresentModeKHR mPresentMode;
        VkSwapchainKHR  mSwapchain;
        VkExtent2D    mSwapchainExtent;
        std::vector<VkImage> mSwapchainImages;
        std::vector<VkImageView> mSwapchainViews;
        
        VkImage         mDepthBuffer;
        VkDeviceMemory  mDepthBufferMemory;
        VkImageView     mDepthBufferView;
        
        VkRenderPass      mRenderPass;
        std::vector<VkFramebuffer>    mFramebuffers;
        
        std::vector<VkFence>          mFences;
        VkSemaphore   mRenderCompletedSem, mPresentCompletedSem;
        
        // デバッグレポート関連
        PFN_vkCreateDebugReportCallbackEXT	mvkCreateDebugReportCallbackEXT;
        PFN_vkDebugReportMessageEXT	mvkDebugReportMessageEXT;
        PFN_vkDestroyDebugReportCallbackEXT mvkDestroyDebugReportCallbackEXT;
        VkDebugReportCallbackEXT  mDebugReport;
        
        std::vector<VkCommandBuffer> mCommands;
        
        uint32_t  mImageIndex;

    };

};