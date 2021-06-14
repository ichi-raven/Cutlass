#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utility.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "RenderPass.hpp"
#include "GraphicsPipeline.hpp"
#include "Command.hpp"
#include "Event.hpp"

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
        WindowInfo() {}

        WindowInfo(const uint32_t width, const uint32_t height, const uint32_t frameCount, const std::string& windowName, bool fullScreen = false, bool vsync = true)
            : width(width)
            , height(height)
            , frameCount(frameCount)
            , windowName(windowName)
            , vsync(vsync)
            , fullScreen(fullScreen)
        {

        }

        WindowInfo(const uint32_t width, const uint32_t height, const uint32_t frameCount, const char* windowName, bool fullScreen = false, bool vsync = true)
            : width(width)
            , height(height)
            , frameCount(frameCount)
            , windowName(std::string(windowName))
            , vsync(vsync)
            , fullScreen(fullScreen)
        {

        }

        uint32_t width;
        uint32_t height;
        uint32_t frameCount;
        std::string windowName;
        bool vsync;
        bool fullScreen;
    };

    class Context
    {
    public:
        Context();
        Context(std::string_view appName, bool debugFlag, Result& result_out);

        ~Context();

        //Noncopyable, Nonmovable
        Context(const Context&) = delete;
        Context &operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context &operator=(Context&&) = delete;

        //明示的に初期化
        Result initialize(std::string_view appName, bool debugFlag);

        //ウィンドウ作成・破棄
        Result createWindow(const WindowInfo& info, HWindow& handle_out);
        Result getWindowSize(const HWindow& handle, uint32_t& width, uint32_t& height);
        Result destroyWindow(const HWindow& handle);

        //バッファ作成・破棄
        Result createBuffer(const BufferInfo& info, HBuffer& handle_out);
        Result destroyBuffer(const HBuffer& handle);

        //バッファ書き込み
        Result writeBuffer(const size_t size, const void *const pData, const HBuffer& handle);
 
        //テクスチャ作成・破棄
        Result createTexture(const TextureInfo& info, HTexture& handle_out);
        Result destroyTexture(const HTexture& handle);

        //ファイルからテクスチャ作成
        Result createTextureFromFile(const char *fileName, HTexture& handle_out);

        //テクスチャにデータ書き込み(使用注意, 書き込むデータのサイズはテクスチャのサイズに従うもの以外危険)
        Result writeTexture(const void *const pData, const HTexture& handle);

        //描画対象オブジェクトをスワップチェインから構築
        Result createRenderPass(const HWindow& handle, bool depthTestEnable, HRenderPass& handle_out);

        //普通に構築
        Result createRenderPass(const RenderPassCreateInfo& info, HRenderPass& handle_out);

        Result destroyRenderPass(const HRenderPass& handle);

        //描画パイプライン構築
        Result createGraphicsPipeline(const GraphicsPipelineInfo& info, HGraphicsPipeline& handle_out);
        Result destroyGraphicsPipeline(const HGraphicsPipeline& handle);
       
        //描画コマンドバッファを作成
        Result createCommandBuffer(const std::vector<CommandList>& commandLists, HCommandBuffer& handle_out);
        Result createCommandBuffer(const CommandList& commandList, HCommandBuffer& handle_out);
        //破棄
        Result destroyCommandBuffer(const HCommandBuffer& handle);
      
        //現在割り当てられているShaderResourceSetの接続を解除する
        Result releaseShaderResourceSet(const HCommandBuffer& handle);

        //すでに割り当てたコマンドの中身を書き換える
        Result rewriteCommandBuffer(const std::vector<CommandList>& commandLists, const HCommandBuffer& handle);
        Result rewriteCommandBuffer(const CommandList& commandList, const HCommandBuffer& handle);

        //現在処理中のフレームバッファのインデックスを取得(0~frameCount)
        uint32_t getFrameBufferIndex(const HRenderPass& handle) const;

        //コマンド実行, バックバッファ表示
        Result execute(const HCommandBuffer& handle);

        //入出力インタフェース
        //各イベントを更新、毎フレーム呼ばないと入力は検知できません
        Result updateInput() const;

        //キー入力取得
        uint32_t getKey(const Key& key) const;
        uint32_t getKey(const HWindow& handle, const Key& key) const;

        //マウス状態取得
        Result getMousePos(double& x, double& y) const;//第1ウィンドウが前提となります
        Result getMousePos(const HWindow& handle, double& x, double& y) const;

        //ウィンドウ終了通知(指定なしで全てのウィンドウの論理和)
        bool shouldClose() const;
        bool shouldClose(const HWindow& handle) const;

        //破棄
        Result destroy();

    private:
        struct WindowObject
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
            // std::vector<VkSemaphore> mRenderCompletedSems;
            // std::vector<VkSemaphore> mPresentCompletedSems;

            //現在のフレーム(注意 : 処理中のフレームバッファのインデックスとは関係ない)
            uint32_t mCurrentFrame;
            //フレームの個数
            uint32_t mMaxFrameNum;
            //同時処理可能なフレーム数
            uint32_t mMaxFrameInFlight;
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
            uint32_t mSizeOfChannel;
            VkFormat format;
            TextureUsage usage;
            VkImageLayout currentLayout;
            VkExtent3D extent;
            VkImageSubresourceRange range;
        };

        struct RenderPassObject
        {
            std::optional<VkRenderPass> mRenderPass;
            std::vector<std::optional<VkFramebuffer>> mFramebuffers;
            std::vector<HTexture> colorTargets;
            std::optional<HTexture> depthTarget;
            std::optional<HWindow> mHWindow;
            DepthStencilState mDSs;
            std::optional<VkExtent3D> mExtent;
            uint32_t mTargetNum;
            bool mDepthTestEnable;
            bool mLoadPrevData;
            //取得されたスワップチェーンイメージのインデックス、テクスチャレンダリングなどしているときは関係ない
            uint32_t mFrameBufferIndex;
            
            //同期オブジェクト, レンダリングの仕方によっては一部しか使用しない
            std::vector<VkFence> mFences;
            //フレーム同時処理用一時的格納場所
            std::vector<VkFence> imagesInFlight;
            std::vector<VkSemaphore> mRenderCompletedSems;
            std::vector<VkSemaphore> mPresentCompletedSems;
        };

        struct GraphicsPipelineObject
        {
            //これ不要説濃厚
            std::optional<VkPipelineLayout> mPipelineLayout;
            std::optional<VkPipeline> mPipeline;
            std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
            std::optional<VkDescriptorPool> mDescriptorPool;
            std::optional<Shader> mVS;
            std::optional<Shader> mFS;
            std::vector<size_t> mSetSizes;//各DescriptorSetのbinding数
            HRenderPass mHRenderPass;
        };

        struct CommandObject
        {
            CommandObject()
            : mPresentFlag(false)
            {}

            std::vector<VkCommandBuffer> mCommandBuffers;
            std::optional<HRenderPass> mHRenderPass;//同じ内容を描画するウィンドウが複数ある場合
            std::optional<HGraphicsPipeline> mHGPO;
            std::vector<std::vector<VkDescriptorSet>> mDescriptorSets;
            bool mPresentFlag;
        };

        static inline Result checkVkResult(VkResult);
        inline Result createInstance();
        inline Result selectPhysicalDevice();
        inline Result createDevice();
        inline Result createCommandPool();

        inline Result createSurface(WindowObject &wo);
        inline Result selectSurfaceFormat(WindowObject &wo, VkFormat format);
        inline Result createSwapchain(WindowObject &wo, bool vsync);
        inline Result createSwapchainImages(WindowObject &wo);
        inline Result createDepthBuffer(WindowObject &wo);

        inline Result searchGraphicsQueueIndex();
        inline uint32_t getMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const;

        inline Result createSyncObjects(RenderPassObject &rdsto);
        
        //描画パスをテクスチャから構築
        inline Result createRenderPass(const std::vector<HTexture>& colors, const bool loadPrevData, HRenderPass& handle_out);
        inline Result createRenderPass(const std::vector<HTexture>& colors, const HTexture& depth, const bool loadPrevData, HRenderPass& handle_out);

        inline Result enableDebugReport();
        inline Result disableDebugReport();
        inline Result setImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

        inline Result createShaderModule(const Shader &shader, const VkShaderStageFlagBits &stage, VkPipelineShaderStageCreateInfo *pSSCI);

        //各コマンド関数
        inline Result cmdBeginRenderPass(CommandObject& co, size_t frameBufferIndex, const CmdBeginRenderPass& info);
        inline Result cmdEndRenderPass(CommandObject& co, const CmdEndRenderPass& info);
        inline Result cmdBindGraphicsPipeline(CommandObject& co, const CmdBindGraphicsPipeline& info);
        inline Result cmdBindVB(CommandObject &co, const CmdBindVB& info);
        inline Result cmdBindIB(CommandObject& co, const CmdBindIB& info);
        inline Result cmdBindSRSet(CommandObject& co, const CmdBindSRSet& info);
        inline Result cmdRenderIndexed(CommandObject& co, const CmdRenderIndexed& info);
        inline Result cmdRender(CommandObject& co, const CmdRender& info);
        inline Result cmdSync(CommandObject& co, const CmdSync& info);

        //アプリケーション名
        std::string mAppName;
        bool mDebugFlag;

        //隠蔽
        HWindow                 mNextWindowHandle;
        HBuffer                 mNextBufferHandle;
        HTexture                mNextTextureHandle;
		HRenderPass				mNextRenderPassHandle;
        HGraphicsPipeline       mNextGPHandle;
        HCommandBuffer          mNextCBHandle;
        std::unordered_map<HWindow, WindowObject>                   mWindowMap;
        std::unordered_map<HBuffer, BufferObject>					mBufferMap;
        std::unordered_map<HTexture, ImageObject>					mImageMap;
        std::unordered_map<HGraphicsPipeline, GraphicsPipelineObject>	mGPMap;
		std::unordered_map<HRenderPass, RenderPassObject>				mRPMap;
        std::unordered_map<HCommandBuffer, CommandObject>           mCommandBufferMap;

        //Vulkan API
        VkInstance mInstance;
        VkDevice mDevice;
        VkPhysicalDevice mPhysDev;
        VkPhysicalDeviceMemoryProperties mPhysMemProps;
        uint32_t mGraphicsQueueIndex;
        VkQueue mDeviceQueue;
        VkCommandPool mCommandPool;

        // デバッグレポート関連
        PFN_vkCreateDebugReportCallbackEXT mvkCreateDebugReportCallbackEXT;
        PFN_vkDebugReportMessageEXT mvkDebugReportMessageEXT;
        PFN_vkDestroyDebugReportCallbackEXT mvkDestroyDebugReportCallbackEXT;
        VkDebugReportCallbackEXT mDebugReport;

        uint32_t mMaxFrame;
        //初期化確認
        bool mIsInitialized;
    };
};