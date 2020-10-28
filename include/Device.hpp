#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.hpp>
#include<vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utility.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "RenderDST.hpp"
#include "RenderPipeline.hpp"
#include "Command.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>

namespace Cutlass
{
    constexpr const char* ENGINE_NAME = "CutlassEngine";

    struct WindowInfo
    {
        uint32_t width;
        uint32_t height;
        std::string windowName;
    };

    struct InitializeInfo
    {
        std::vector<WindowInfo> windows;
        std::string appName;
        uint32_t frameCount;
        bool debugFlag;
    };

    class Device
    {
    public:
        Device() : mMaxFrameNum(0),
                   mNextSwapchainHandle(1),
                   mNextBufferHandle(1),
                   mNextTextureHandle(1),
                   mNextRenderDSTHandle(1),
                   mNextRPHandle(1),
                   mNextCBHandle(1)
        {
        }

        ~Device() {}

        //初期化
        Result initialize(const InitializeInfo &info, std::vector<HSwapchain> &handlesRef);

        //バッファ作成
        Result createBuffer(const BufferInfo &info, HBuffer *pHandle);

        //バッファ書き込み
        Result writeBuffer(const size_t size, const void *const pData, const HBuffer &handle);

        //テクスチャ作成
        Result createTexture(const TextureInfo &info, HTexture *pHandle);

        //ファイルからテクスチャ作成
        Result createTextureFromFile(const char *fileName, HTexture *pHandle);

        //テクスチャにデータ書き込み(使用注意, 書き込むデータのサイズはテクスチャのサイズに従うもの以外危険)
        Result writeTexture(const void *const pData, const HTexture &handle);

        //用途変更
        Result changeTextureUsage(TextureUsage prev, TextureUsage next, const HTexture *pHandle);

        //描画対象オブジェクトをスワップチェインから構築
        Result createRenderDSTFromSwapchain(const HSwapchain &handle, bool depthTestEnable, HRenderDST *pHandle);

        //描画対象オブジェクトをテクスチャから構築
        Result createRenderDSTFromTextures(const std::vector<HTexture> textures, HRenderDST *pHandle);

        //描画パイプライン構築
        Result createRenderPipeline(const RenderPipelineInfo &info, HRenderPipeline *pHandle);

        Result createCommandBuffer(const CommandList& commandList, HCommandBuffer* const pHandle);

        //コマンド記述
        //Result writeCommand(CommandType, CommandInfo, const HCommandList& handle);

        //コマンド実行, バックバッファ表示
        Result execute(const HCommandBuffer& handle, const HSwapchain& swapchain);

        //無
        //Result present();

        //全部破棄
        Result destroy();
        //TODO:フラグによる未セット時のデストラクタに置けるDestroy

    private:
        struct SwapchainObject
        {
            std::optional<GLFWwindow*> mpWindow;
            std::optional<VkSurfaceKHR> mSurface;
            std::optional<VkSwapchainKHR> mSwapchain;
            VkSurfaceCapabilitiesKHR mSurfaceCaps;
            VkSurfaceFormatKHR mSurfaceFormat;
            VkPresentModeKHR mPresentMode;
            VkExtent2D mSwapchainExtent;
            std::vector<HTexture> mHSwapchainImages;
            HTexture mHDepthBuffer;
        };

        struct BufferObject
        {
            std::optional<VkBuffer> mBuffer;
            std::optional<VkDeviceMemory> mMemory;
            bool mIsHostVisible;
        };

        struct ImageObject
        {
            std::optional<VkImage> mImage;
            std::optional<VkDeviceMemory> mMemory;
            std::optional<VkImageView> mView;
            std::optional<VkSampler> mSampler;
            bool mIsHostVisible;
            VkFormat format;
            TextureUsage usage;
            VkExtent3D extent;
        };

        struct RenderDSTObject
        {
            std::optional<VkRenderPass> mRenderPass;
            std::vector<std::optional<VkFramebuffer>> mFramebuffers;
            std::optional<HSwapchain> mHSwapchain;
            DepthStencilState mDSs;
            std::optional<VkExtent3D> mExtent;
            uint32_t mTargetNum;
        };

        struct RenderPipelineObject
        {
            //これ不要説濃厚
            std::optional<VkPipelineLayout> mPipelineLayout;
            std::optional<VkPipeline> mPipeline;
            std::optional<VkDescriptorSetLayout> mDescriptorSetLayout;
            std::optional<VkDescriptorPool> mDescriptorPool;
            ShaderResourceSetLayout layout; //firstがユニフォームバッファ, シェーダリソースの接続管轄用
            HRenderDST mHRenderDST;
        };

        struct CommandObject
        {//バッファリングは自動で行う
            std::vector<VkCommandBuffer> mCommandBuffers;
            std::vector<VkFence> mFences;
            std::optional<HRenderDST> mHRenderDST;
            std::optional<HRenderPipeline> mHRPO;
            std::vector<VkDescriptorSet> mDescriptorSets;
        };

        static inline Result checkVkResult(VkResult);
        Result createInstance();
        Result selectPhysicalDevice();
        Result createDevice();
        Result createCommandPool();
        Result createCommandBuffers();

        Result createSurface(SwapchainObject *pSO);
        Result selectSurfaceFormat(SwapchainObject *pSO, VkFormat format);
        Result createSwapchain(SwapchainObject *pSO);
        Result createSwapchainImages(SwapchainObject *pSO);
        Result createDepthBuffer(SwapchainObject *pSO);

        Result createSemaphores();

        Result searchGraphicsQueueIndex();
        uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const;

        Result enableDebugReport();
        Result disableDebugReport();
        Result setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        Result createShaderModule(const Shader &shader, const VkShaderStageFlagBits &stage, VkPipelineShaderStageCreateInfo *pSSCI);

        //各コマンド関数
        inline Result cmdBeginRenderPipeline(CommandObject& co, const CmdBeginRenderPipeline& info);
        inline Result cmdEndRenderPipeline(CommandObject& co, const CmdEndRenderPipeline &info);
        inline Result cmdBindVB(CommandObject& co,const CmdBindVB& info);
        inline Result cmdBindIB(CommandObject& co, const CmdBindIB &info);
        inline Result cmdBindSRSet(CommandObject& co, const CmdBindSRSet &info);
        inline Result cmdRenderIndexed(CommandObject& co, const CmdRenderIndexed &info);
        inline Result cmdRender(CommandObject& co, const CmdRender &info);

        //ユーザ指定
        InitializeInfo mInitializeInfo;

        //隠蔽
        HSwapchain              mNextSwapchainHandle;
        HBuffer                 mNextBufferHandle;
        HTexture                mNextTextureHandle;
		HRenderDST				mNextRenderDSTHandle;
        HRenderPipeline         mNextRPHandle;
        HCommandBuffer          mNextCBHandle;
        std::unordered_map<HSwapchain, SwapchainObject>             mSwapchainMap;
        std::unordered_map<HBuffer, BufferObject>					mBufferMap;
        std::unordered_map<HTexture, ImageObject>					mImageMap;
        std::unordered_map<HRenderPipeline, RenderPipelineObject>	mRPMap;
		std::unordered_map<HRenderDST, RenderDSTObject>				mRDSTMap;
        std::unordered_map<HCommandBuffer, CommandObject>           mCommandBufferMap;

        //std::vector<Command> mWroteCommands;

        //Vulkan API
        VkInstance mInstance;
        VkDevice mDevice;
        VkPhysicalDevice mPhysDev;
        VkPhysicalDeviceMemoryProperties mPhysMemProps;
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        VkCommandPool mCommandPool;

        std::vector<VkSemaphore> mRenderCompletedSems;
        std::vector<VkSemaphore> mPresentCompletedSems;
        //std::vector<VkFence> mFences;
        //std::vector<VkCommandBuffer> mCommands;

        // デバッグレポート関連
        PFN_vkCreateDebugReportCallbackEXT mvkCreateDebugReportCallbackEXT;
        PFN_vkDebugReportMessageEXT mvkDebugReportMessageEXT;
        PFN_vkDestroyDebugReportCallbackEXT mvkDestroyDebugReportCallbackEXT;
        VkDebugReportCallbackEXT mDebugReport;

        //現在のフレーム(処理用、Swapchainのインデックスとは関係ない)
        uint32_t mCurrentFrame;
        //フレームの個数
        uint32_t mMaxFrameNum;
        //同時処理可能なフレーム数
        uint32_t mMaxFrameInFlight;

        //フレーム同時処理用一時的格納場所
        std::vector<VkFence> imagesInFlight;
    };
};