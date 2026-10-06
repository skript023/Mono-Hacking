#include "hooking.hpp"
#include "unity/crafting_tools.hpp"
#include "unity/item_data.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include <format>

namespace big
{
	bool hooks::player_recipe_requirements(MonoObject* player, MonoObject* recipe, bool discover, int quality, int amount)
	{
		if (player && recipe && crafting_tools::active(player))
			return true;
		return detour_base::get_original<player_recipe_requirements>()(player, recipe, discover, quality, amount);
	}
	bool hooks::player_piece_requirements(MonoObject* player, MonoObject* piece, int mode)
	{
		if (player && piece && crafting_tools::active(player))
			return true;
		return detour_base::get_original<player_piece_requirements>()(player, piece, mode);
	}
	bool hooks::player_check_can_remove_piece(MonoObject* player, MonoObject* piece)
	{
		if (player && crafting_tools::active(player))
			return true;
		return detour_base::get_original<player_check_can_remove_piece>()(player, piece);
	}
	bool hooks::required_crafting_station(MonoObject* player, MonoObject* recipe, int quality, bool check_level)
	{
		if (player && recipe && crafting_tools::active(player))
			return true;
		return detour_base::get_original<required_crafting_station>()(player, recipe, quality, check_level);
	}
	void hooks::consume_resources(MonoObject* player, MonoArray* requirements, int quality, int item_quality, int multiplier)
	{
		if (player && crafting_tools::active(player))
			return;
		detour_base::get_original<consume_resources>()(player, requirements, quality, item_quality, multiplier);
	}
	MonoObject* hooks::recipe_required_station(MonoObject* recipe, int quality)
	{
		if (recipe && crafting_tools::active())
			return nullptr;
		return detour_base::get_original<recipe_required_station>()(recipe, quality);
	}
	MonoObject* hooks::first_required_item(MonoObject* player, MonoObject* inventory, MonoObject* recipe, int quality, int* amount, int* extra, int multiplier)
	{
		if (player && crafting_tools::active(player))
			if (auto item = crafting_tools::ingredient(recipe, amount, extra))
				return item;
		return detour_base::get_original<first_required_item>()(player, inventory, recipe, quality, amount, extra, multiplier);
	}
	bool hooks::inventory_gui_can_repair(MonoObject* gui, MonoObject* item)
	{
		if (crafting_tools::active())
			return true;
		return detour_base::get_original<inventory_gui_can_repair>()(gui, item);
	}
	bool hooks::inventory_gui_have_repairable_items(MonoObject* gui)
	{
		if (crafting_tools::active())
			return true;
		return detour_base::get_original<inventory_gui_have_repairable_items>()(gui);
	}
	void hooks::inventory_gui_repair_one_item(MonoObject* gui)
	{
		if (crafting_tools::active())
		{
			auto player_obj = unity::get_local_player();
			if (!player_obj)
				return;

			player p(player_obj);
			auto inv = p.get_inventory();
			auto inv_obj = inv.get_object();
			if (!inv_obj)
				return;

			auto inv_class = mono::object_get_class(inv_obj);
			auto f_inv = mono::get_field(inv_class, "m_inventory");
			if (!f_inv)
				return;

			MonoObject* list_obj = nullptr;
			mono::get_field_value(inv_obj, f_inv, &list_obj);
			if (!list_obj)
				return;

			auto items = unity::list_to_vector(list_obj);
			for (auto item : items)
			{
				if (!item)
					continue;

				item_data id(item);
				float cur_dur = id.get_durability();
				float max_dur = id.get_max_durability();
				if (max_dur > 0.f && cur_dur < max_dur)
				{
					id.set_durability(max_dur);
					auto shared_obj = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
					std::string name = "Item";
					if (shared_obj)
					{
						auto ms = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_name", MonoString*>(shared_obj);
						if (ms)
							name = mono::from_mono_string(ms);
					}
					unity::show_message(std::format("Repaired {}", name));
					return;
				}
			}
			unity::show_message("No more items to repair");
			return;
		}

		detour_base::get_original<inventory_gui_repair_one_item>()(gui);
	}
}
