#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Utility.hpp"

namespace Cutlass
{
	struct RenderDSTInfo//スワップチェーンへの描画かテクスチャへの描画かどちらかを指定してください
	{
		std::optional<HSwapchain> hSwapchain;
		std::optional<std::vector<HTexture>> renderTargets;//同一フォーマット同一サイズとかじゃないと誤動作します
		
	};
}