#pragma once
#ifdef RENDERER_SMOKE_TEST
	#include "renderer_test_common.hpp"
#else
	#include "common.hpp"
#endif
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
