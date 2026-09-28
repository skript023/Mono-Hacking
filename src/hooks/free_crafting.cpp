#include "hooking.hpp"
#include "unity/crafting_tools.hpp"

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
}
