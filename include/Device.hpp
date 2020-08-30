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

        //バッファ作成
        Result createBuffer(const BufferInfo& info, HBuffer *pHandle);

        //バッファ書き込み
        Result writeBuffer(const size_t size, const void *const pData, const HBuffer& handle);

        //テクスチャ作成
        Result createTexture(const TextureInfo& info, HTexture *pHandle);
        
        //ファイルからテクスチャ作成
        Result createTextureFromFile(const char* fileName, HTexture *pHandle);

        //テクスチャにデータ書き込み(使用注意, 書き込むデータのサイズはテクスチャのサイズに従うもの以外危険)
        Result writeTexture(const void *const pData, const HTexture& handle);

        //テクスチャのusage変更
        Result changeTextureUsage(const HTexture* pHandle);

        //用途変更
        Result changeTextureUsage(TextureUsage prev, TextureUsage next, const HTexture* pHandle);

		//描画対象オブジェクトをスワップチェインから構築
		Result createRenderDSTFromSwapchain(const HSwapchain& handle, bool depthTestEnable, HRenderDST* pHandle);

        //描画対象オブジェクトをテクスチャから構築
        Result createRenderDSTFromTextures(const std::vector<HTexture> textures, HRenderDST *pHandle);

        //描画パイプライン構築
        Result createRenderPipeline(const RenderPipelineInfo& info, HRenderPipeline* pHandle);

        //コマンド記述
        Result writeCommand(const Command& command);

        //コマンド実行
        Result execute();

        //バックバッファ表示
        Result present(const HSwapchain& handle);
        
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
            std::vector<HTexture>       mHSwapchainImages;
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
            std::optional<HSwapchain>   mHSwapchain;
            DepthStencilState mDSs;
            std::optional<VkExtent3D> mExtent;
            uint32_t                mTargetNum;
        };

        struct RenderPipelineObject
        {
            //これ不要説濃厚
            std::optional<VkPipelineLayout>        mPipelineLayout;
            std::optional<VkPipeline>              mPipeline;
            std::optional<VkDescriptorSetLayout>   mDescriptorSetLayout;
            std::optional<VkDescriptorPool>        mDescriptorPool;
            std::pair<uint32_t, uint32_t>          mDescriptorCount;//firstがユニフォームバッファ, シェーダリソースの接続管轄用
            HRenderDST                             mHRenderDST;
        };

        struct RunningCommandState
        {
            std::optional<HRenderPipeline> mHRPO;
            std::optional<VkDescriptorSet> mDescriptorSet;
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

        Result createSemaphores();

        Result searchGraphicsQueueIndex();
        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps)const;

        Result enableDebugReport();
        Result disableDebugReport();
        Result setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        Result createShaderModule(Shader shader, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo* pSSCI);

        //各コマンド関数
        Result cmdBeginRenderPipeline(const CmdBeginRenderPipeline& info);
        Result cmdEndRenderPipeline(const CmdEndRenderPipeline& info);
        Result cmdSetVB(const CmdSetVB& info);
        Result cmdSetIB(const CmdSetIB& info);
        Result cmdSetShaderResource(const CmdSetShaderResource& info);
        Result cmdRender(const CmdRender &info);

        //ユーザ指定
        InitializeInfo mInitializeInfo;

        //隠蔽
        HSwapchain              mNextSwapchainHandle;
        HBuffer                 mNextBufferHandle;
        HTexture                mNextTextureHandle;
		HRenderDST				mNextRenderDSTHandle;
        HRenderPipeline         mNextRPHandle;
        std::unordered_map<HSwapchain, SwapchainObject>				mSwapchainMap;
        std::unordered_map<HBuffer, BufferObject>					mBufferMap;
        std::unordered_map<HTexture, ImageObject>					mImageMap;
        std::unordered_map<HRenderPipeline, RenderPipelineObject>	mRPMap;
		std::unordered_map<HRenderDST, RenderDSTObject>				mRDSTMap;

        std::optional<RunningCommandState> mRCState;

        //Vulkan
        VkInstance  mInstance;
        VkDevice    mDevice;
        VkPhysicalDevice  mPhysDev;
        VkPhysicalDeviceMemoryProperties mPhysMemProps;
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        VkCommandPool mCommandPool;
        
        std::vector<VkFence>          mFences;
        VkSemaphore                   mRenderCompletedSem;
        VkSemaphore                   mPresentCompletedSem;
        std::vector<VkCommandBuffer>  mCommands;

        // デバッグレポート関連
        PFN_vkCreateDebugReportCallbackEXT	mvkCreateDebugReportCallbackEXT;
        PFN_vkDebugReportMessageEXT	mvkDebugReportMessageEXT;
        PFN_vkDestroyDebugReportCallbackEXT mvkDestroyDebugReportCallbackEXT;
        VkDebugReportCallbackEXT  mDebugReport;
        
        //現在のフレームが指すスワップチェインイメージ
        uint32_t  mFrameIndex;
    };

};