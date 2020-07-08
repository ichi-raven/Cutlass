#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.hpp>
#include<vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

#include "Utility.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "RenderPipeline.hpp"
#include "Command.hpp"

namespace Cutlass
{
    #define ENGINE_NAME ("CutlassEngine")

    struct windowInfo
    {
        uint32_t width;
        uint32_t height;
        std::string windowName;
    };

    struct InitializeInfo
    {
        std::vector<windowInfo>     windows;
        std::string                 appName;
        uint32_t                    frameCount;
        bool                        debugFlag;
    };

    class Device
    {
    public:
        Device() : mNextSwapchainHandle(1),
                   mNextBufferHandle(1),
                   mNextTextureHandle(1),
                   mNextSamplerHandle(1),
                   mNextRPHandle(1)
                   {}

        ~Device();

        //初期化
        Result initialize(const InitializeInfo& info, std::vector<HSwapchain>* hSwapchains);

        //スワップチェインのテクスチャハンドル取得(指定したフレーム数)
        Result getSwapchainImages(HSwapchain handle, std::vector<HTexture>* hSwapchainImages);

        //バッファ作成
        Result createBuffer(const BufferInfo& info, HBuffer *pHandle);

        //バッファ書き込み
        Result writeBuffer(const HBuffer& handle, void *data, size_t bytesize);

        //テクスチャ作成(ファイル読み込み可)
        Result createTexture(const TextureInfo& info, HTexture *pHandle);
        Result createTextureFromFile(const char* fileName, HTexture *pHandle);

        //データ書き込み(動かない)
        Result writeTexture(const size_t size, const void* const pData, const HTexture& handle);

        //用途変更
        Result changeUsage(TextureUsage prev, TextureUsage next, const HTexture &handle);

        //サンプラー作成
        Result createSampler(HSampler *pHandle);

        //画像に使用するサンプラーをアタッチ
        Result attachSampler(const HTexture &handle, const HSampler &hSampler);

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

        struct SwapchainObject
        {
            std::optional<GLFWwindow*> mpWindow;

            std::optional<VkSurfaceKHR>             mSurface;
            std::optional<VkSwapchainKHR>           mSwapchain;

            VkSurfaceCapabilitiesKHR    mSurfaceCaps;
            VkSurfaceFormatKHR          mSurfaceFormat;
            VkPresentModeKHR            mPresentMode;
            VkExtent2D                  mSwapchainExtent;
            std::vector<HTexture>       mSwapchainImages;
        };

        struct BufferObject
        {
            std::optional<VkBuffer>        mBuffer;
            std::optional<VkDeviceMemory>  mMemory;
            bool mIsHostVisible;
        };

        struct ImageObject
        {
            std::optional<VkImage>          mImage;
            std::optional<VkDeviceMemory>   mMemory;
            std::optional<VkImageView>      mView;
            std::optional<HSampler>         mHSampler;
            std::optional<VkFramebuffer>    mFrameBuffer;
            bool                            mIsHostVisible;
            VkFormat                        format;
            TextureUsage                    usage;
        };

        struct RenderPipelineObject
        {
            std::optional<VkRenderPass>            mRenderPass;
            std::optional<VkPipelineLayout>        mPipelineLayout;
            std::optional<VkPipeline>              mPipeline;
            std::optional<VkDescriptorSetLayout>   mDescriptorSetLayout;
        };


        static inline Result checkVkResult(VkResult);
        Result createInstance();
        Result selectPhysicalDevice();
        Result createDevice();
        Result createCommandPool();
        Result createCommandBuffers();

        Result createSurface(SwapchainObject* pSO);
        Result selectSurfaceFormat(SwapchainObject* pSO, VkFormat format);
        Result createSwapchain(SwapchainObject* pSO);

        //Result createDepthBuffer();
        Result createSwapchainImages(SwapchainObject* pSO);

        Result createDefaultFramebuffer();

        Result createSemaphores();

        Result searchGraphicsQueueIndex();
        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps)const;

        Result enableDebugReport();
        Result disableDebugReport();
        Result setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        //ユーザ指定
        InitializeInfo mInitializeInfo;

        //隠蔽
        HSwapchain              mNextSwapchainHandle;
        HBuffer                 mNextBufferHandle;
        HTexture                mNextTextureHandle;
        HSampler                mNextSamplerHandle;
        HRenderPipeline         mNextRPHandle;
        std::unordered_map<HSwapchain, SwapchainObject> mSwapchainMap;
        std::unordered_map<HBuffer, BufferObject> mBufferMap;
        std::unordered_map<HTexture, ImageObject> mImageMap;
        std::unordered_map<HRenderPipeline, RenderPipelineObject> mRPMap;
        std::unordered_map<HSampler, VkSampler> mSamplerMap;


        VkInstance  mInstance;
        VkDevice    mDevice;
        VkPhysicalDevice  mPhysDev;

        VkPhysicalDeviceMemoryProperties mPhysMemProps;

        // VkSurfaceKHR        mSurface;
        // VkSurfaceFormatKHR  mSurfaceFormat;
        // VkSurfaceCapabilitiesKHR  mSurfaceCaps;
        
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        
        VkCommandPool mCommandPool;
        // VkPresentModeKHR mPresentMode;
        // VkSwapchainKHR  mSwapchain;
        // VkExtent2D    mSwapchainExtent;
        // std::vector<VkImage> mSwapchainImages;
        // std::vector<VkImageView> mSwapchainViews;

        ImageObject mDepthBuffer;
        
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