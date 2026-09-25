#include "common.hpp"
#include "function_types.hpp"
#include "astra/host/canvas.hpp"
#include "hooking.hpp"
#include "memory/module.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#include "graphic/graphic_manager.hpp"

#include <MinHook.h>

namespace big
{
	template<auto callback>
	void add_mono_hook(std::string_view name, const char* class_name, const char* method_name, int parameter_count, const char* assembly, const char* namespace_name = "")
	{
		LOG(INFO) << "Resolving hook: " << name;
		Logger::FlushQueue();
		auto target = mono::get_compile_method(class_name, method_name, parameter_count, assembly, namespace_name);
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		while (!target)
		{
			if (!g_running)
				throw std::runtime_error("Hook initialization cancelled.");
			if (std::chrono::steady_clock::now() >= deadline)
				throw std::runtime_error(std::format("Hook '{}' could not resolve after 10 seconds; check the game method signature.", name));
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			target = mono::get_compile_method(class_name, method_name, parameter_count, assembly, namespace_name);
		}
		detour_hook::add<callback>(name, target);
	}

	hooking::hooking()
	try
	{
		if (graphic_manager::get_method_table(hooks::swapchain_present_index))
		{
			detour_hook::add<hooks::swapchain_present>("SwapChainPresent", graphic_manager::get_method_table(hooks::swapchain_present_index));
			detour_hook::add<hooks::swapchain_resizebuffers>("SwapChainResizeBuffers", graphic_manager::get_method_table(hooks::swapchain_resizebuffers_index));
		}


		detour_hook::add<hooks::set_cursor_pos>("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>());
		detour_hook::add<hooks::convert_thread_to_fiber>("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>());

		add_mono_hook<hooks::is_teleportable>("Inventory::IsTeleportable", "Inventory", "IsTeleportable", 1, "assembly_valheim");
		add_mono_hook<hooks::update>("Player::Update", "Player", "Update", 0, "assembly_valheim");
		add_mono_hook<hooks::create_tomb_stone>("Player::CreateTombStone", "Player", "CreateTombStone", 0, "assembly_valheim");
		add_mono_hook<hooks::is_debug_flying>("Player::IsDebugFlying", "Player", "IsDebugFlying", 0, "assembly_valheim");
		add_mono_hook<hooks::update_guardian_power>("Player::UpdateGuardianPower", "Player", "UpdateGuardianPower", 1, "assembly_valheim");
		add_mono_hook<hooks::is_under_roof>("Cover::IsUnderRoof", "Cover", "IsUnderRoof", 1, "assembly_utils");
		add_mono_hook<hooks::update_water>("Character::UpdateWater", "Character", "UpdateWater", 1, "assembly_valheim");
		add_mono_hook<hooks::on_selected_item>("InventoryGui::OnSelectedItem", "InventoryGui", "OnSelectedItem", 4, "assembly_valheim");
		add_mono_hook<hooks::get_weight>("ItemDrop::ItemData::GetWeight", "ItemDrop/ItemData", "GetWeight", 1, "assembly_valheim");
		add_mono_hook<hooks::set_alerted>("AnimalAI::SetAlerted", "AnimalAI", "SetAlerted", 1, "assembly_valheim");
		add_mono_hook<hooks::is_wind_controll_active>("Ship::IsWindControllActive", "Ship", "IsWindControllActive", 0, "assembly_valheim");
		add_mono_hook<hooks::is_out_of_water>("Fish::IsOutOfWater", "Fish", "IsOutOfWater", 0, "assembly_valheim");
		add_mono_hook<hooks::raise_skill>("Player::RaiseSkill", "Player", "RaiseSkill", 2, "assembly_valheim");
		add_mono_hook<hooks::take_input>("PlayerController::TakeInput", "PlayerController", "TakeInput", 1, "assembly_valheim");
		add_mono_hook<hooks::get_body_armor>("Player::GetBodyArmor", "Player", "GetBodyArmor", 0, "assembly_valheim");
		add_mono_hook<hooks::have_empty_slot>("Inventory::HaveEmptySlot", "Inventory", "HaveEmptySlot", 0, "assembly_valheim");
		add_mono_hook<hooks::allowed_command>("Terminal::ConsoleCommand::IsValid", "Terminal/ConsoleCommand", "IsValid", 2, "assembly_valheim");
		add_mono_hook<hooks::camera_render>("Camera::Render", "Camera", "Render", 0, "UnityEngine.CoreModule", "UnityEngine");
		add_mono_hook<hooks::get_projectile_spawn_point>("Attack::GetProjectileSpawnPoint", "Attack", "GetProjectileSpawnPoint", 2, "assembly_valheim");
		add_mono_hook<hooks::on_hit>("Projectile::OnHit", "Projectile", "OnHit", 4, "assembly_valheim");
		add_mono_hook<hooks::is_known_material>("Player::IsKnownMaterial", "Player", "IsKnownMaterial", 1, "assembly_valheim");
		add_mono_hook<hooks::drop_item>("ItemDrop::DropItem", "ItemDrop", "DropItem", 4, "assembly_valheim");
		add_mono_hook<hooks::rpc_use_stamina>("Player::RPC_UseStamina", "Player", "RPC_UseStamina", 2, "assembly_valheim");
		add_mono_hook<hooks::on_map_middle_click>("Minimap::OnMapMiddleClick", "Minimap", "OnMapMiddleClick", 1, "assembly_valheim");
		add_mono_hook<hooks::top_first>("Inventory::TopFirst", "Inventory", "TopFirst", 1, "assembly_valheim");
		add_mono_hook<hooks::wearntear_get_support>("WearNTear::GetSupport", "WearNTear", "GetSupport", 0, "assembly_valheim");
		add_mono_hook<hooks::wearntear_have_support>("WearNTear::HaveSupport", "WearNTear", "HaveSupport", 0, "assembly_valheim");
		add_mono_hook<hooks::private_area_check_access>("PrivateArea::CheckAccess", "PrivateArea", "CheckAccess", 4, "assembly_valheim");
		add_mono_hook<hooks::get_attack_draw_percentage>("Humanoid::GetAttackDrawPercentage", "Humanoid", "GetAttackDrawPercentage", 0, "assembly_valheim");
		add_mono_hook<hooks::is_weapon_loaded>("Player::IsWeaponLoaded", "Player", "IsWeaponLoaded", 0, "assembly_valheim");
		add_mono_hook<hooks::character_rpc_damage>("Character::RPC_Damage", "Character", "RPC_Damage", 2, "assembly_valheim");
		add_mono_hook<hooks::player_in_god_mode>("Player::InGodMode", "Player", "InGodMode", 0, "assembly_valheim");
		add_mono_hook<hooks::player_in_ghost_mode>("Player::InGhostMode", "Player", "InGhostMode", 0, "assembly_valheim");
		add_mono_hook<hooks::player_no_cost_cheat>("Player::NoCostCheat", "Player", "NoCostCheat", 0, "assembly_valheim");
		add_mono_hook<hooks::smelter_delta>("Smelter::GetDeltaTime", "Smelter", "GetDeltaTime", 0, "assembly_valheim");
		add_mono_hook<hooks::fermenter_time>("Fermenter::GetFermentationTime", "Fermenter", "GetFermentationTime", 0, "assembly_valheim");
		add_mono_hook<hooks::hive_delta>("Beehive::GetTimeSinceLastUpdate", "Beehive", "GetTimeSinceLastUpdate", 0, "assembly_valheim");
		add_mono_hook<hooks::plant_grow_time>("Plant::GetGrowTime", "Plant", "GetGrowTime", 0, "assembly_valheim");
		add_mono_hook<hooks::environment_override>("EnvMan::GetEnvironmentOverride", "EnvMan", "GetEnvironmentOverride", 0, "assembly_valheim");
		add_mono_hook<hooks::environment_update>("EnvMan::FixedUpdate", "EnvMan", "FixedUpdate", 0, "assembly_valheim");
		add_mono_hook<hooks::container_stack_response>("Container::RPC_StackResponse", "Container", "RPC_StackResponse", 2, "assembly_valheim");
		add_mono_hook<hooks::inventory_add_stack_item>("Inventory::AddItem(ItemData)", "Inventory", "AddItem", 1, "assembly_valheim");
		// Older game versions do not have the cheat label or the six-argument overload.
		if (auto tooltip = mono::get_compile_method("ItemDrop/ItemData", "GetTooltip", 6, "assembly_valheim"))
			detour_hook::add<hooks::item_get_tooltip>("ItemDrop::ItemData::GetTooltip", tooltip);
		add_mono_hook<hooks::humanoid_drain_durability>("Humanoid::DrainEquipedItemDurability", "Humanoid", "DrainEquipedItemDurability", 2, "assembly_valheim");
		add_mono_hook<hooks::player_get_run_speed_factor>("Player::GetRunSpeedFactor", "Player", "GetRunSpeedFactor", 0, "assembly_valheim");
		add_mono_hook<hooks::player_get_jog_speed_factor>("Player::GetJogSpeedFactor", "Player", "GetJogSpeedFactor", 0, "assembly_valheim");
		add_mono_hook<hooks::attack_modify_damage>("Attack::ModifyDamage", "Attack", "ModifyDamage", 2, "assembly_valheim");
		add_mono_hook<hooks::player_can_eat>("Player::CanEat", "Player", "CanEat", 2, "assembly_valheim");
		add_mono_hook<hooks::player_eat_food>("Player::EatFood", "Player", "EatFood", 1, "assembly_valheim");

		g_hooking = this;
	}

	catch (const std::exception& e)
	{
		LOG(WARNING) << "Hook initialization failed: " << e.what();
		Logger::FlushQueue();
		while (!detour_base::hooks().empty())
			delete detour_base::hooks().back();
		throw;
	}

	hooking::~hooking()
	{
		if (m_enabled)
			disable();

		while (!detour_base::hooks().empty())
			delete detour_base::hooks().back();

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

		while (!detour_base::hooks().empty())
			delete detour_base::hooks().back();
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
			std::lock_guard lock(render_mutex);
			g_renderer->wndproc(hwnd, msg, wparam, lparam);
			if (canvas::captures_message(msg))
			{
				if (msg == WM_INPUT)
					return DefWindowProcW(hwnd, msg, wparam, lparam);
				return 0;
			}
		}

		return CallWindowProcW(g_hooking->m_og_wndproc, hwnd, msg, wparam, lparam);
	}

	BOOL hooks::set_cursor_pos(int x, int y)
	{
		if (canvas::uses_mouse())
			return true;

		return detour_base::get_original<hooks::set_cursor_pos>()(x, y);
	}
}
