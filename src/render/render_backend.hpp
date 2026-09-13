#pragma once
#include "common.hpp"
#include <imgui.h>
namespace big
{
	inline std::recursive_mutex render_mutex;

	class render_backend
	{
	public:
		virtual ~render_backend() = default;
		virtual void shutdown() = 0;
		virtual ImTextureID upload_rgba(const unsigned char* pixels, int width, int height) = 0;
	};
}
