#pragma once

#ifdef RENDERER_SMOKE_TEST
	#include "renderer_test_common.hpp"
#else
	#include "common.hpp"
#endif
#include "render_backend.hpp"

namespace big
{
	// Device entry points are resolved exclusively through a dummy Vulkan device.
	class render_vulkan final : public render_backend
	{
	public:
		static bool stop_capture();
		static void attach(uint32_t vendor_id = 0, uint32_t device_id = 0);
		static void detach();
		void shutdown() override;
		ImTextureID upload_rgba(const unsigned char* pixels, int width, int height) override;
		void release_texture(ImTextureID texture) override;

	private:
		static void initialize_loader();
	};
}
