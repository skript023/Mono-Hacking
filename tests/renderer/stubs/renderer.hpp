#pragma once
#include "renderer_test_common.hpp"
#include "render_backend.hpp"
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
namespace big
{
	enum class renderer_api
	{
		unknown,
		dx11,
		vulkan
	};
	struct renderer
	{
		HWND m_window{};
		renderer_api active = renderer_api::unknown;
		ImTextureID texture{};
		int draws = 0;
		bool m_init = false;
		renderer_api api() const
		{
			return active;
		}
		bool begin_vulkan(render_backend*)
		{
			if (!ImGui::GetCurrentContext())
			{
				ImGui::CreateContext();
				ImGui::GetIO().IniFilename = nullptr;
				ImGui_ImplWin32_Init(m_window);
			}
			active = renderer_api::vulkan;
			return true;
		}
		void finish_init();
		void release_vulkan()
		{
			m_init = false;
			texture = 0;
		}
		void draw_frame()
		{
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			auto list = ImGui::GetBackgroundDrawList();
			list->AddRectFilled({8, 8}, {32, 32}, IM_COL32(255, 0, 0, 255));
			if (texture)
				list->AddImage(texture, {40, 8}, {64, 32});
			list->AddText({8, 40}, IM_COL32_WHITE, "renderer smoke");
			ImGui::Render();
			++draws;
		}
	};
	inline renderer* g_renderer{};
}
