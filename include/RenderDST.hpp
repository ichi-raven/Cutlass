#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Utility.hpp"

namespace Cutlass
{

	struct RenderDSTInfo //スワップチェインへの描画かテクスチャへの描画かどちらかを指定してください
	{
		std::optional<HSwapchain> hSwapchain;
		std::optional<std::vector<HTexture>> hRenderTargets;
		bool depthTestEnable;//スワップチェインを利用する場合以外意味がないです
	};
}