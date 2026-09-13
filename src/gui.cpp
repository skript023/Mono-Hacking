#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <textures/textures.hpp>

#include "gui.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#include "memory/module.hpp"
#include "memory/pattern.hpp"

#include <imgui.h>
#include "ui/canvas.hpp"
#include "javascript/bindings/gui_binding.hpp"

#include "menu/view.hpp"
#include "menu/esp/esp.h"

//#include <input/input_service.hpp>
#include "notification/notification_service.hpp"

namespace big
{
	/**
	 * @brief The later an entry comes in this enum to higher up it comes in the z-index.
	 */
	enum eRenderPriority
	{
		// low priority
		ESP,
		CENSOR,
		JS_GUI,

		// medium priority
		MENU = 0x1000,
		VEHICLE_CONTROL,
		JS,

		// high priority
		INFO_OVERLAY = 0x2000,
		INPUT,
		CMD_EXECUTOR,

		GTA_DATA_CACHE = 0x3000,
		ONBOARDING,

		// should remain in a league of its own
		NOTIFICATIONS = 0x4000,
	};

	void gui::init()
	{
		this->add_dx_callback(esp::draw_esp, eRenderPriority::ESP);
		this->add_dx_callback(view::draw_input, eRenderPriority::INPUT);
		this->add_dx_callback(view::notifications, eRenderPriority::NOTIFICATIONS);
		//this->add_dx_callback(view::draw_overlay, eRenderPriority::INFO_OVERLAY);
		this->add_dx_callback([this] { this->dx_on_opened(); }, eRenderPriority::MENU);
		this->add_dx_callback(view::js_scripts, eRenderPriority::JS);
		this->add_dx_callback(js_gui::register_gui, eRenderPriority::JS_GUI);

		this->add_wndproc_callback([this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { wndproc(hwnd, msg, wparam, lparam); });


		view::register_submenu();
		LOG(INFO) << "DirectX Callback Registered.";
	}
	void gui::dx_init()
	{
		static ImVec4 bgColor = ImVec4(0.09f, 0.094f, 0.129f, .9f);
		static ImVec4 primary = ImVec4(0.172f, 0.380f, 0.909f, 1.f);
		static ImVec4 secondary = ImVec4(0.443f, 0.654f, 0.819f, 1.f);
		static ImVec4 whiteBroken = ImVec4(0.792f, 0.784f, 0.827f, 1.f);

		auto& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(15, 15);
		style.WindowRounding = 10.f;
		style.WindowBorderSize = 0.f;
		style.FramePadding = ImVec2(5, 5);
		style.FrameRounding = 4.0f;
		style.ItemSpacing = ImVec2(12, 8);
		style.ItemInnerSpacing = ImVec2(8, 6);
		style.IndentSpacing = 25.0f;
		style.ScrollbarSize = 15.0f;
		style.ScrollbarRounding = 9.0f;
		style.GrabMinSize = 5.0f;
		style.GrabRounding = 3.0f;
		style.ChildRounding = 4.0f;

		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(g_settings.window.color);
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(g_settings.window.color);
		colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.30f, 0.29f, 0.32f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	}

	bool gui::add_dx_callback(dx_callback callback, uint32_t priority)
	{
		if (!m_dx_callbacks.insert({ priority, callback }).second)
		{
			LOG(WARNING) << "Duplicate priority given on DX Callback!";

			return false;
		}
		return true;
	}

	void gui::add_wndproc_callback(wndproc_callback&& callback)
	{
		m_wndproc_callbacks.emplace_back(std::move(callback));
	}


	void gui::dx_on_opened()
	{
		canvas::tick();
	}

	void gui::script_init()
	{
		notification::success("Welcome", std::format("{} Trainer Successfully Injected. Press insert to open", GAME_NAME));
	}

	void gui::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYDOWN && wparam == VK_ESCAPE)
		{
			//if (g_input_service.is_open()) g_input_service.hide();
		}
		canvas::check_for_input();
		canvas::handle_input();
	}

    void gui::load_textures()
    {
        auto load = [](const unsigned char* bytes, int size, ImTextureID& id, ImageDimensions& dimensions)
        {
            int width = 0, height = 0;
            auto pixels = stbi_load_from_memory(bytes, size, &width, &height, nullptr, 4);
            if (!pixels) { LOG(WARNING) << "Cannot decode menu texture"; return; }
            id = g_renderer->upload_rgba(pixels, width, height);
            stbi_image_free(pixels);
            if (id) dimensions = {width, height};
            else LOG(WARNING) << "Cannot upload menu texture";
        };
        load(quantum_green, sizeof(quantum_green), m_header, m_header_size);
        load(toggle_texture, sizeof(toggle_texture), m_toggle, m_toggle_size);
    }
}
