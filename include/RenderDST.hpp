#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Utility.hpp"

namespace Cutlass
{
	struct RenderDSTInfo//�X���b�v�`�F�[���ւ̕`�悩�e�N�X�`���ւ̕`�悩�ǂ��炩���w�肵�Ă�������
	{
		std::optional<HSwapchain> hSwapchain;
		std::optional<std::vector<HTexture>> renderTargets;//����t�H�[�}�b�g����T�C�Y�Ƃ�����Ȃ��ƌ듮�삵�܂�
		
	};
}