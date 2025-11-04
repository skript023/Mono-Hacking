#include "common.hpp"
#include "function_types.hpp"
#include "logger.hpp"
#include "ui/canvas.hpp"
#include "hooking.hpp"
#include "memory/module.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#include "graphic/graphic_manager.hpp"

#include <MinHook.h>

namespace big
{
	hooking::hooking()
	{
		detour_base::add<hooks::swapchain_present>(new detour_hook("SwapChainPresent", graphic_manager::get_method_table(hooks::swapchain_present_index), hooks::swapchain_present));
		detour_base::add<hooks::swapchain_resizebuffers>(new detour_hook("SwapChainResizeBuffers", graphic_manager::get_method_table(hooks::swapchain_resizebuffers_index), hooks::swapchain_resizebuffers));

		detour_base::add<hooks::set_cursor_pos>(new detour_hook("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>(), hooks::set_cursor_pos));
		detour_base::add<hooks::convert_thread_to_fiber>(new detour_hook("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>(), hooks::convert_thread_to_fiber));
		
		detour_base::add<hooks::is_teleportable>(new detour_hook("Humanoid::IsTeleportable", mono::get_compile_method("Humanoid", "IsTeleportable", 0, "assembly_valheim"), hooks::is_teleportable));
		detour_base::add<hooks::create_tomb_stone>(new detour_hook("Player::CreateTombStone", mono::get_compile_method("Player", "CreateTombStone", 0, "assembly_valheim"), hooks::create_tomb_stone));
		detour_base::add<hooks::is_under_roof>(new detour_hook("Cover::IsUnderRoof", mono::get_compile_method("Cover", "IsUnderRoof", 1, "assembly_utils"), hooks::is_under_roof));
		detour_base::add<hooks::on_selected_item>(new detour_hook("InventoryGui::OnSelectedItem", mono::get_compile_method("InventoryGui", "OnSelectedItem", 4, "assembly_valheim"), hooks::on_selected_item));

		g_hooking = this;
	}

	hooking::~hooking()
	{
		if (m_enabled)
			disable();

		g_hooking = nullptr;
	}

	void hooking::enable()
	{
		m_og_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&hooks::wndproc)));

		detour_base::enable_all();

		MH_ApplyQueued();

		m_enabled = true;
	}

	void hooking::disable()
	{
		m_enabled = false;

		SetWindowLongPtrW(g_pointers->m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_og_wndproc));

		detour_base::disable_all();
		MH_ApplyQueued();

		for (auto it : detour_base::hooks())
		{
			delete it;
		}
	}

	minhook_keepalive::minhook_keepalive()
	{
		MH_Initialize();
	}

	minhook_keepalive::~minhook_keepalive()
	{
		MH_Uninitialize();
	}

	void* hooks::convert_thread_to_fiber(void* param)
	{
		if (IsThreadAFiber())
		{
			return GetCurrentFiber();
		}

		return detour_base::get_original<hooks::convert_thread_to_fiber>()(param);
	}

	LRESULT hooks::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (g_running)
		{
			g_renderer->wndproc(hwnd, msg, wparam, lparam);
		}

		return CallWindowProcW(g_hooking->m_og_wndproc, hwnd, msg, wparam, lparam);
	}

	BOOL hooks::set_cursor_pos(int x, int y)
	{
		if (canvas::is_opened())
			return true;

		return detour_base::get_original<hooks::set_cursor_pos>()(x, y);
	}
}
