#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "menu_settings.hpp"
#include "unity/equipment_slots.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool hooks::humanoid_equip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects)
	{
		TRY_CLAUSE
		{
			if (g_running && g_settings.self.equipment_slots_enabled && !equipment_manager::get().is_in_equip())
			{
				if (equipment_manager::get().handle_inventory_equip(humanoid, item))
					return true;

				equipment_manager::get().on_equip_item(humanoid, item, trigger_equip_effects);
			}
			return detour_base::get_original<humanoid_equip_item>()(humanoid, item, trigger_equip_effects);
		}
		EXCEPT_CLAUSE

		return detour_base::get_original<humanoid_equip_item>()(humanoid, item, trigger_equip_effects);
	}

	void hooks::humanoid_unequip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects)
	{
		TRY_CLAUSE
		{
			if (g_running && g_settings.self.equipment_slots_enabled && !equipment_manager::get().is_in_equip())
			{
				if (equipment_manager::get().on_unequip_item(humanoid, item, trigger_equip_effects))
					return;
			}
			detour_base::get_original<humanoid_unequip_item>()(humanoid, item, trigger_equip_effects);
			return;
		}
		EXCEPT_CLAUSE

		detour_base::get_original<humanoid_unequip_item>()(humanoid, item, trigger_equip_effects);
	}

	void hooks::humanoid_update_equipment_status_effects(MonoObject* humanoid)
	{
		TRY_CLAUSE
		{
			detour_base::get_original<humanoid_update_equipment_status_effects>()(humanoid);
			if (g_running && g_settings.self.equipment_slots_enabled && g_settings.self.multi_utility_enabled && !equipment_manager::get().is_in_equip())
			{
				equipment_manager::get().on_update_equipment_status_effects(humanoid);
			}
			return;
		}
		EXCEPT_CLAUSE

		detour_base::get_original<humanoid_update_equipment_status_effects>()(humanoid);
	}

	void hooks::humanoid_unequip_all_items(MonoObject* humanoid)
	{
		TRY_CLAUSE
		{
			if (g_running && g_settings.self.equipment_slots_enabled)
			{
				equipment_manager::get().on_unequip_all(humanoid);
			}
			detour_base::get_original<humanoid_unequip_all_items>()(humanoid);
			return;
		}
		EXCEPT_CLAUSE

		detour_base::get_original<humanoid_unequip_all_items>()(humanoid);
	}

	void hooks::player_use_hotbar_item(MonoObject* player, int index)
	{
		TRY_CLAUSE
		{
			if (g_running && g_settings.self.equipment_slots_enabled && g_settings.self.equipment_slots_hotkeys)
			{
				bool alt_held = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
				bool shift_held = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

				if (alt_held && index >= 1 && index <= 6)
				{
					equipment_manager::get().use_slot(slot_kind::quick, index);
					return;
				}
				if (shift_held && index >= 1 && index <= 3)
				{
					equipment_manager::get().use_slot(slot_kind::food, index);
					return;
				}
			}
			detour_base::get_original<player_use_hotbar_item>()(player, index);
			return;
		}
		EXCEPT_CLAUSE

		detour_base::get_original<player_use_hotbar_item>()(player, index);
	}

	void hooks::player_save(MonoObject* player, MonoObject* pkg)
	{
		TRY_CLAUSE
		{
			if (g_running && g_settings.self.equipment_slots_enabled)
			{
				equipment_manager::get().on_player_save(player, pkg);
			}
			detour_base::get_original<player_save>()(player, pkg);
			return;
		}
		EXCEPT_CLAUSE

		detour_base::get_original<player_save>()(player, pkg);
	}

	bool hooks::inventory_contains_item(MonoObject* inventory, MonoObject* item)
	{
		TRY_CLAUSE
		{
			if (detour_base::get_original<inventory_contains_item>()(inventory, item))
				return true;

			if (g_running && g_settings.self.equipment_slots_enabled && item)
			{
				auto player = unity::get_local_player();
				if (player)
				{
					auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
					if (inventory == player_inv)
					{
						auto eq = equipment_manager::get().get_equipment_inventory();
						if (eq && eq != inventory)
						{
							return detour_base::get_original<inventory_contains_item>()(eq, item);
						}
					}
				}
			}
			return false;
		}
		EXCEPT_CLAUSE

		return detour_base::get_original<inventory_contains_item>()(inventory, item);
	}

	bool hooks::inventory_remove_item(MonoObject* inventory, MonoObject* item, int amount)
	{
		TRY_CLAUSE
		{
			if (detour_base::get_original<inventory_remove_item>()(inventory, item, amount))
				return true;

			if (g_running && g_settings.self.equipment_slots_enabled && item)
			{
				auto player = unity::get_local_player();
				if (player)
				{
					auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
					if (inventory == player_inv)
					{
						auto eq = equipment_manager::get().get_equipment_inventory();
						if (eq && eq != inventory)
						{
							bool removed = detour_base::get_original<inventory_remove_item>()(eq, item, amount);
							if (removed)
							{
								equipment_manager::get().save_to_player();
							}
							return removed;
						}
					}
				}
			}
			return false;
		}
		EXCEPT_CLAUSE

		return detour_base::get_original<inventory_remove_item>()(inventory, item, amount);
	}
}
