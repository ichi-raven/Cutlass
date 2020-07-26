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
#include "RenderDST.hpp"
#include "RenderPipeline.hpp"
#include "Command.hpp"


namespace Cutlass
{
    #define ENGINE_NAME ("CutlassEngine")

    struct WindowInfo
    {
        uint32_t width;
        uint32_t height;
        std::string windowName;
    };

    struct InitializeInfo
    {
        std::vector<WindowInfo>     windows;
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
                   mNextRenderDSTHandle(1),
                   mNextRPHandle(1)
                   {}

        ~Device();

        //初期化
        Result initialize(const InitializeInfo& info, std::vector<HSwapchain>& handlesRef);

        // //スワップチェインのテクスチャハンドル取得(指定フレーム)
        // Result getSwapchainImageHandle(HSwapchain handle, std::vector<HTexture>& handlesRef);

        // //スワップチェインのデプスバッファを取得
        // Result getSwapchainDepthBuffer(HSwapchain handle, HTexture *pHandle);

        //バッファ作成
        Result createBuffer(const BufferInfo& info, HBuffer *pHandle);

        //バッファ書き込み(未実装)
        Result writeBuffer(HBuffer handle, void *data, size_t bytesize);

        //ファイルからシェーダリソ−ステクスチャ作成
        Result createTexture(const TextureInfo& info, HTexture *pHandle);
        //
        Result createTextureFromFile(const char* fileName, HTexture *pHandle);

        Result changeTextureUsage(const HTexture* pHandle);

        //データ書き込み(未実装)
        Result writeTexture(const size_t size, const void* const pData, const HTexture* pHandle);

        //用途変更
        Result changeTextureUsage(TextureUsage prev, TextureUsage next, const HTexture* pHandle);

        //サンプラー作成
        //Result createSampler(HSampler *pHandle);

        //画像に使用するサンプラーをアタッチ
        //Result attachSampler(HTexture handle, const HSampler &hSampler);

		//描画対象オブジェクト構築
		Result createRenderDSTFromSwapchain(const HSwapchain& handle, bool depthTestEnable, HRenderDST* pHandle);

        Result createRenderDSTFromTextures(const std::vector<HTexture> textures, HRenderDST *pHandle);

        //描画パイプライン構築
        Result createRenderPipeline(const RenderPipelineInfo& info, HRenderPipeline* pHandle);

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
            std::optional<GLFWwindow*>      mpWindow;
            std::optional<VkSurfaceKHR>     mSurface;
            std::optional<VkSwapchainKHR>   mSwapchain;

            VkSurfaceCapabilitiesKHR    mSurfaceCaps;
            VkSurfaceFormatKHR          mSurfaceFormat;
            VkPresentModeKHR            mPresentMode;
            VkExtent2D                  mSwapchainExtent;
            std::vector<HTexture>       mSwapchainImages;
            HTexture                    mHDepthBuffer;
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
            std::optional<VkSampler>        mSampler;
            bool                            mIsHostVisible;
            VkFormat                        format;
            TextureUsage                    usage;
            VkExtent3D                      extent;
        };


		struct RenderDSTObject
		{
			std::optional<VkRenderPass> mRenderPass;
			std::vector<std::optional<VkFramebuffer>> mFramebuffers;
            bool mIsTargetSwapchain;
            DepthStencilState mDSs;
            std::optional<VkExtent3D> mExtent;
        };

        struct RenderPipelineObject
        {
            //これ不要説濃厚
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
        Result createSwapchainImages(SwapchainObject* pSO);
        Result createDepthBuffer(SwapchainObject* pSO);

        Result createDefaultFramebuffer();

        Result createSemaphores();

        Result searchGraphicsQueueIndex();
        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps)const;

        Result enableDebugReport();
        Result disableDebugReport();
        Result setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        Result createShaderModule(Shader shader, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo* pSSCI);

        //ユーザ指定
        InitializeInfo mInitializeInfo;

        //隠蔽
        HSwapchain              mNextSwapchainHandle;
        HBuffer                 mNextBufferHandle;
        HTexture                mNextTextureHandle;
        //HSampler                mNextSamplerHandle;
		HRenderDST				mNextRenderDSTHandle;
        HRenderPipeline         mNextRPHandle;
        std::unordered_map<HSwapchain, SwapchainObject>				mSwapchainMap;
        std::unordered_map<HBuffer, BufferObject>					mBufferMap;
        std::unordered_map<HTexture, ImageObject>					mImageMap;
        std::unordered_map<HRenderPipeline, RenderPipelineObject>	mRPMap;
		std::unordered_map<HRenderDST, RenderDSTObject>				mRDSTMap;
        //std::unordered_map<HSampler, VkSampler> mSamplerMap;


        VkInstance  mInstance;
        VkDevice    mDevice;
        VkPhysicalDevice  mPhysDev;
        VkPhysicalDeviceMemoryProperties mPhysMemProps;
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        VkCommandPool mCommandPool;

        //ImageObject mDepthBuffer;
        
        std::vector<VkFence>          mFences;
        VkSemaphore   mRenderCompletedSem, mPresentCompletedSem;
        std::vector<VkCommandBuffer> mCommands;

        // デバッグレポート関連
        PFN_vkCreateDebugReportCallbackEXT	mvkCreateDebugReportCallbackEXT;
        PFN_vkDebugReportMessageEXT	mvkDebugReportMessageEXT;
        PFN_vkDestroyDebugReportCallbackEXT mvkDestroyDebugReportCallbackEXT;
        VkDebugReportCallbackEXT  mDebugReport;

        
        uint32_t  mSwapchainImageIndex;//現在のフレームが指すスワップチェインイメージ
    };

};