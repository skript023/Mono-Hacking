#include "common.hpp"
#include "function_types.hpp"
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
		detour_hook::add<hooks::swapchain_present>("SwapChainPresent", graphic_manager::get_method_table(hooks::swapchain_present_index));
		detour_hook::add<hooks::swapchain_resizebuffers>("SwapChainResizeBuffers", graphic_manager::get_method_table(hooks::swapchain_resizebuffers_index));

		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());
		
		detour_hook::add<hooks::is_teleportable>("Humanoid::IsTeleportable", mono::get_compile_method("Humanoid", "IsTeleportable", 0, "assembly_valheim"));
		detour_hook::add<hooks::update>("Player::Update", mono::get_compile_method("Player", "Update", 0, "assembly_valheim"));
		detour_hook::add<hooks::create_tomb_stone>("Player::CreateTombStone", mono::get_compile_method("Player", "CreateTombStone", 0, "assembly_valheim"));
		detour_hook::add<hooks::is_debug_flying>("Player::IsDebugFlying", mono::get_compile_method("Player", "IsDebugFlying", 0, "assembly_valheim"));
		detour_hook::add<hooks::update_guardian_power>("Player::UpdateGuardianPower", mono::get_compile_method("Player", "UpdateGuardianPower", 1, "assembly_valheim"));
		detour_hook::add<hooks::is_under_roof>("Cover::IsUnderRoof", mono::get_compile_method("Cover", "IsUnderRoof", 1, "assembly_utils"));
		detour_hook::add<hooks::update_water>("Character::UpdateWater", mono::get_compile_method("Character", "UpdateWater", 1, "assembly_valheim"));
		detour_hook::add<hooks::on_selected_item>("InventoryGui::OnSelectedItem", mono::get_compile_method("InventoryGui", "OnSelectedItem", 4, "assembly_valheim"));
		detour_hook::add<hooks::get_weight>("ItemDrop::ItemData::GetWeight", mono::get_compile_method("ItemDrop/ItemData", "GetWeight", 1, "assembly_valheim"));
		detour_hook::add<hooks::set_alerted>("AnimalAI::SetAlerted", mono::get_compile_method("AnimalAI", "SetAlerted", 1, "assembly_valheim"));
		detour_hook::add<hooks::is_wind_controll_active>("Ship::IsWindControllActive", mono::get_compile_method("Ship", "IsWindControllActive", 0, "assembly_valheim"));
		detour_hook::add<hooks::is_out_of_water>("Fish::IsOutOfWater", mono::get_compile_method("Fish", "IsOutOfWater", 0, "assembly_valheim"));
		detour_hook::add<hooks::raise_skill>("Player::RaiseSkill", mono::get_compile_method("Player", "RaiseSkill", 2, "assembly_valheim"));
		detour_hook::add<hooks::take_input>("PlayerController::TakeInput", mono::get_compile_method("PlayerController", "TakeInput", 1, "assembly_valheim"));
		detour_hook::add<hooks::get_body_armor>("Player::GetBodyArmor", mono::get_compile_method("Player", "GetBodyArmor", 0, "assembly_valheim"));
		detour_hook::add<hooks::have_empty_slot>("Inventory::HaveEmptySlot", mono::get_compile_method("Inventory", "HaveEmptySlot", 0, "assembly_valheim"));
		detour_hook::add<hooks::allowed_command>("Terminal::ConsoleCommand::IsValid", mono::get_compile_method("Terminal/ConsoleCommand", "IsValid", 2, "assembly_valheim"));
		detour_hook::add<hooks::camera_render>("Camera::Render", mono::get_compile_method("Camera", "Render", 0, "UnityEngine.CoreModule", "UnityEngine"));
		detour_hook::add<hooks::get_projectile_spawn_point>("Attack::GetProjectileSpawnPoint", mono::get_compile_method("Attack", "GetProjectileSpawnPoint", 2, "assembly_valheim"));
		detour_hook::add<hooks::on_hit>("Projectile::OnHit", mono::get_compile_method("Projectile", "OnHit", 4, "assembly_valheim"));

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
