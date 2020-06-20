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

#include <vector>
#include <unordered_map>
#include <optional>

namespace Cutlass
{
    #define ENGINE_NAME ("CutlassEngine")

    using HBuffer = uint64_t;
    using HTexture = uint64_t;
    using HSampler = uint64_t;
    using HRenderPipeline = uint64_t;

    enum class Result
    {
        eSuccess = 0,
        eFailure,
    };

    enum class ResourceType
    {
        eUint = 1,
        eInt,
        eVec2,
        eVec3,
        eVec4,
        eMat2,
        eMat3,
        eMat4,
    };

    enum class RenderDSTType
    {
        eColor,
        eDepth,
    };

    enum class BufferUsage
    {
        eVertex,
        eIndex,
        eUniform,
    };

    enum class TextureUsage
    {
        eShaderResource,
        eRenderTarget,
        eUnordered,
    };

    enum class ColorBlend
    {
        eDefault = 0,
    };

    enum class RasterizerState
    {
        eDefault = 0,
    };

    enum class MultiSampleState
    {
        eDefault = 0,
    };

    enum class DepthStencilState
    {
        eDefault = 0,
    };

    struct VertexLayout
    {
        std::vector<ResourceType> layouts;
        std::vector<std::string> names;
    };

    struct ShaderResourceLayout
    {
        uint32_t uniformBufferCount;
        uint32_t combinedTextureCount;
    };

    struct ShaderResource
    {
        std::vector<HBuffer>    uniformBuffer;
        std::vector<HTexture>   combinedTexture;
    };

    struct RenderPipelineInfo
    {
        VertexLayout                vertexLayout;
        ColorBlend                  colorBlend;
        RasterizerState             rasterizerState;
        MultiSampleState            multiSampleState;
        std::vector<RenderDSTType>  renderDSTs;
        std::string                 vertexShaderPath;
        std::string                 fragmentShaderPath;
        ShaderResourceLayout        SRLayouts;
    };

    struct InitializeInfo
    {
        uint32_t    width;
        uint32_t    height;
        std::string appName;
        uint32_t    frameCount;
        bool        debugFlag;
    };

    struct Command
    {
        std::optional<HBuffer> hVertexBuffer;
        std::optional<HBuffer> hIndexBuffer;
        ShaderResource         shaderResource;
        uint32_t               firstIndex;
        uint32_t               indexCount;
        uint32_t               instanceCount;
        uint32_t               vertexOffset;
        uint32_t               firstInstance;
    };

    class Device
    {
    public:
        Device();
        ~Device();

        //初期化
        Result initialize(const InitializeInfo& initializeInfo);

        //バッファ作成
        Result createBuffer(BufferUsage bufferUsage, size_t HBuffer *hBuffer);

        //バッファ書き込み
        Result writeBuffer(const HBuffer& hBuffer, void *data, size_t bytesize);

        //テクスチャ作成(ファイル読み込み可)
        Result createTexture(TextureUsage textureUsage, HTexture *hTexture);
        Result createTexture(const std::string &fileName, TextureUsage textureUsage, HTexture *hTexture);

        //サンプラー作成
        Result createSampler(HSampler *hSampler);

        //画像に使用するサンプラーをアタッチ
        Result attachSampler(const HTexture &hTexture, const HSampler &hSampler);

        //描画パイプライン構築
        Result createRenderPipeline(const RenderPipelineInfo& info, HRenderPipeline* hRenderPipeline);

        //コマンド記述
        Result writeCommand(const Command& command);

        //コマンド送信
        Result submitCommand();

        //バックバッファ表示
        Result present();

        //破棄
        Result destroy();

    private:
    
        struct BufferObject
        {
            VkBuffer        mBuffer;
            VkDeviceMemory  mMemory;
        };

        struct ImageObject
        {
            VkImage             mImage;
            VkDeviceMemory      mMemory;
            VkImageView         mView;
            HSampler            mHSampler;
        };

        struct RenderPipelineObject
        {
            VkRenderPass            mRenderPass;
            VkPipelineLayout        mPipelineLayout;
            VkPipeline              mPipeline;
            VkDescriptorSetLayout   mDescriptorSetLayout;
        };

        static inline Result checkResult(VkResult);
        Result initializeInstance();

        Result selectPhysicalDevice();
        Result createDevice();
        Result createCommandPool();
        Result selectSurfaceFormat(VkFormat format);
        Result createSwapchain(GLFWwindow* window);
        Result createDepthBuffer();
        Result createViews();

        Result createFramebuffer();

        Result createSemaphores();

        uint32_t searchGraphicsQueueIndex();
        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps)const;

        Result enableDebugReport();
        Result disableDebugReport();


        //ユーザ指定
        InitializeInfo mInitializeInfo;


        //API
        GLFWwindow* mWindow;

        VkInstance  mInstance;
        VkDevice    mDevice;
        VkPhysicalDevice  mPhysDev;

        VkPhysicalDeviceMemoryProperties mPhysMemProps;

        VkSurfaceKHR        mSurface;
        VkSurfaceFormatKHR  mSurfaceFormat;
        VkSurfaceCapabilitiesKHR  mSurfaceCaps;
        
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
        
        //VkRenderPass      mRenderPass;
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