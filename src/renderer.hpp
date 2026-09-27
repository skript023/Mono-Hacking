#pragma once
#include "common.hpp"
#include <imgui.h>
#include "render/render_backend.hpp"
namespace big
{
	enum class renderer_api
	{
		unknown,
		dx11,
		vulkan
	};
	class render_dx11;
	class renderer
	{
		std::unique_ptr<render_dx11> m_dx11;
		render_backend* m_backend = nullptr;
		renderer_api m_api = renderer_api::unknown;
		uint64_t m_texture_generation = 0;

	public:
		renderer();
		~renderer();
		void attach();
		// DLLs are candidates only. Confirm the API from an actual game presentation.
		static bool has_vulkan()
		{
			return GetModuleHandleW(L"vulkan-1.dll") != nullptr;
		}
		static bool has_dx11()
		{
			return GetModuleHandleW(L"d3d11.dll") != nullptr;
		}
		renderer_api api() const
		{
			return m_api;
		}
		bool m_init = false;
		bool init(IDXGISwapChain* swapchain);
		void imgui_init();
		void on_present(IDXGISwapChain* swapchain);
		void pre_reset();
		void post_reset(IDXGISwapChain* swapchain);
		bool owns_dx11(IDXGISwapChain* swapchain) const;
		bool begin_vulkan(render_backend* backend);
		void finish_init();
		void release_vulkan();
		void draw_frame(ImVec2 framebuffer_size = {});
		ImTextureID upload_rgba(const unsigned char* pixels, int width, int height);
		void release_texture(ImTextureID texture);
		uint64_t texture_generation() const
		{
			return m_texture_generation;
		}
		void merge_icon_with_latest_font(float font_size, bool owned = false);
		void wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		ImFont* m_font = nullptr;
		ImFont* m_ui_manager_font = nullptr;
		ImFont* m_monospace_font = nullptr;
		HWND m_window = nullptr;
	};
	inline renderer* g_renderer{};
}
