#include "commands/number_command.hpp"
#include "features/features.hpp"
#include "hooking.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include <script_mgr.hpp>
#include "unity/equipment_slots.hpp"
#include "unity/item_drop.hpp"
#include "utility/unity.hpp"

namespace big
{
	using namespace features;

	number_command<float> _durability("durability", "Item Durability", "", 1.f, 10000.f, 100.f);
	number_command<int> _quality("quality", "Item Quality", "", 1, 100, 1);
	number_command<int> _stack("stack", "Item Stack", "", 1, 100, 1);
	number_command<int> _variant("variant", "Item Variant", "", 1, 100, 0);

	void hooks::on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod)
	{
		if (g_running && g_settings.self.equipment_slots_enabled)
		{
			auto eq_grid = equipment_manager::get().get_native_grid();
			if (eq_grid && grid_ptr == eq_grid)
			{
				auto gui = static_cast<MonoObject*>(this_ptr);
				if (gui)
				{
					auto drag_item = mono::get_field_value<"InventoryGui", "m_dragItem", MonoObject*>(gui);
					if (drag_item != nullptr)
					{
						slot_kind kind = equipment_manager::get_slot_kind(pos.x, pos.y);
						if (kind == slot_kind::none || !equipment_manager::fits_slot(kind, drag_item))
						{
							notification::warning("Equipment Slots", "Item cannot be placed in this slot.");
							return;
						}
					}
				}
			}
		}

		if (_override_selected.get_state())
		{
			if (item_data_obj != nullptr)
			{
				item_data data(item_data_obj);

				data.set_durability(_durability.get_state());
				data.set_quality(_quality.get_state());
				data.set_stack(_stack.get_state());
				data.set_variant(_variant.get_state());
			}
		}

		detour_base::get_original<on_selected_item>()(this_ptr, grid_ptr, item_data_obj, pos, mod);

		if (g_running && g_settings.self.equipment_slots_enabled)
		{
			auto eq_grid = equipment_manager::get().get_native_grid();
			if (eq_grid && grid_ptr == eq_grid)
			{
				auto player = unity::get_local_player();
				if (player)
				{
					equipment_manager::get().sync_equipment_with_humanoid(player);
					equipment_manager::get().save_to_player();
				}
			}
		}
	}
}
