#include "renderer.hpp"
#include "render/render_dx11.hpp"
#include "render/render_vulkan.hpp"
#include "gui.hpp"
#include "pointers.hpp"
#include "mono/mono.hpp"
#include "file_manager.hpp"
#include "fonts/font_list.hpp"
#include "fonts/icon_list.hpp"
#include "graphic/graphic_manager.hpp"
#include "astra/host/canvas.hpp"
#include "unity/item_icons.hpp"
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace big
{
	renderer::renderer()
	{
		m_window = g_pointers->m_hwnd;
		if (has_dx11() && graphic_manager::get_swapchain(eGraphicsAPI::directx11) == eInitializationStatus::SUCCESS)
			m_dx11 = std::make_unique<render_dx11>();
		LOG(INFO) << "Renderer candidates: DX11=" << bool(m_dx11) << ", Vulkan=" << has_vulkan();
		g_gui.init();
		g_renderer = this;
	}
	void renderer::attach()
	{
		if (!has_vulkan())
			return;

		auto get_property = [](const char* name) {
			auto method = mono::get_compile_method("SystemInfo", name, 0, "UnityEngine.CoreModule", "UnityEngine");
			return mono::invoke_compiled_method<int>(method);
		};

		// Match the dummy device to Unity's selected adapter on multi-GPU systems.
		const auto vendor_id = get_property("get_graphicsDeviceVendorID");
		const auto device_id = get_property("get_graphicsDeviceID");
		render_vulkan::attach(static_cast<uint32_t>(vendor_id), static_cast<uint32_t>(device_id));
	}

	renderer::~renderer()
	{
		render_vulkan::detach();
		std::lock_guard lock(render_mutex);
		if (m_dx11)
			m_dx11->shutdown();
		if (ImGui::GetCurrentContext())
		{
			if (ImGui::GetIO().BackendPlatformUserData)
				ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
		g_renderer = nullptr;
	}
	bool renderer::init(IDXGISwapChain* swapchain)
	{
		if (!m_dx11 || m_api == renderer_api::vulkan)
			return false;
		DXGI_SWAP_CHAIN_DESC desc{};
		if (FAILED(swapchain->GetDesc(&desc)) || desc.OutputWindow != m_window)
			return false;
		if (!ImGui::GetCurrentContext())
			imgui_init();
		if (!m_dx11->init(swapchain))
			return false;
		m_backend = m_dx11.get();
		m_api = renderer_api::dx11;
		finish_init();
		LOG(INFO) << "Active renderer: DX11";
		return true;
	}
	bool renderer::owns_dx11(IDXGISwapChain* swapchain) const
	{
		return m_api == renderer_api::dx11 && m_dx11 && m_dx11->owns(swapchain);
	}
	bool renderer::begin_vulkan(render_backend* backend)
	{
		if (m_api == renderer_api::dx11)
			return false;
		if (!ImGui::GetCurrentContext())
			imgui_init();
		m_backend = backend;
		m_api = renderer_api::vulkan;
		return true;
	}
	void renderer::release_vulkan()
	{
		if (m_api != renderer_api::vulkan)
			return;
		m_init = false;
		m_backend = nullptr;
		g_gui.m_header = g_gui.m_toggle = 0;
	}
	void renderer::finish_init()
	{
		++m_texture_generation;
		g_gui.load_textures();
		m_init = true;
		static bool notified = false;
		if (!notified)
		{
			g_gui.script_init();
			notified = true;
		}
	}
	void renderer::on_present(IDXGISwapChain* swapchain)
	{
		std::lock_guard lock(render_mutex);
		if (owns_dx11(swapchain) || init(swapchain))
			m_dx11->present();
	}
	void renderer::pre_reset()
	{
		std::lock_guard lock(render_mutex);
		if (m_dx11 && m_init)
			m_dx11->pre_reset();
	}
	void renderer::post_reset(IDXGISwapChain* swapchain)
	{
		std::lock_guard lock(render_mutex);
		if (m_dx11 && m_init)
			m_dx11->post_reset(swapchain);
	}
	ImTextureID renderer::upload_rgba(const unsigned char* pixels, int width, int height)
	{
		return m_backend ? m_backend->upload_rgba(pixels, width, height) : 0;
	}
	void renderer::release_texture(ImTextureID texture)
	{
		if (m_backend && texture)
			m_backend->release_texture(texture);
	}
	void renderer::draw_frame(ImVec2 framebuffer_size)
	{
		item_icons::begin_frame();
		auto& io = ImGui::GetIO();
		io.MouseDrawCursor = canvas::uses_mouse();
		if (canvas::uses_mouse())
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		else
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
		ImGui_ImplWin32_NewFrame();
		if (framebuffer_size.x > 0.f && framebuffer_size.y > 0.f)
		{
			if (io.DisplaySize.x <= 0.f || io.DisplaySize.y <= 0.f)
				io.DisplaySize = framebuffer_size;
			io.DisplayFramebufferScale = {framebuffer_size.x / io.DisplaySize.x, framebuffer_size.y / io.DisplaySize.y};
		}
		ImGui::NewFrame();
		for (const auto& cb : g_gui.m_dx_callbacks | std::views::values)
			cb();
		ImGui::Render();
	}
	void renderer::imgui_init()
	{
		auto file_path = file_manager::get_project_file("./imgui.ini").get_path();

		ImGuiContext* ctx = ImGui::CreateContext();

		static std::string path = file_path.make_preferred().string();
		ctx->IO.IniFilename = path.c_str();

		if (!ImGui_ImplWin32_Init(m_window))
			throw std::runtime_error("Cannot initialize ImGui Win32 backend");

		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		std::strcpy(font_cfg.Name, "Rubik");

		m_font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(font_rubik), sizeof(font_rubik), 17.f, &font_cfg);
		merge_icon_with_latest_font(14.f, false);

		m_monospace_font = ImGui::GetIO().Fonts->AddFontDefault();


		g_gui.dx_init();
	}
	void renderer::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		std::lock_guard lock(render_mutex);
		if (msg == WM_KEYUP && wparam == VK_INSERT)
		{
			//Persist and restore the cursor position between menu instances.
			static POINT cursor_coords{};
			if (canvas::is_opened())
			{
				GetCursorPos(&cursor_coords);
			}
			else if (cursor_coords.x + cursor_coords.y != 0)
			{
				SetCursorPos(cursor_coords.x, cursor_coords.y);
			}
		}
		if (msg == WM_KEYUP && wparam == VK_END)
		{
			g_running = false;
		}


		if (m_init && ImGui::GetCurrentContext())
		{
			ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
		}
	}

	void renderer::merge_icon_with_latest_font(float font_size, bool FontDataOwnedByAtlas)
	{
		static const ImWchar icons_ranges[3] = {ICON_MIN_FA, ICON_MAX_FA, 0};

		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.FontDataOwnedByAtlas = FontDataOwnedByAtlas;

		g_settings.window.font_icon = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font_icons, sizeof(font_icons), font_size, &icons_config, icons_ranges);
	}
}
