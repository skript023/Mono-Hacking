#include "hooking.hpp"
#include "unity/localization.hpp"
#include "script.hpp"

namespace big
{
	MonoString* hooks::item_get_tooltip(MonoObject* item, int quality, bool crafting, float world_level, int stack_override, bool appending)
	{
		TRY_CLAUSE
		{
			static bool logged_entry = false;
			if (!logged_entry)
			{
				logged_entry = true;
				LOG(INFO) << "Item tooltip filter invoked.";
			}
			auto result = detour_base::get_original<item_get_tooltip>()(item, quality, crafting, world_level, stack_override, appending);
			if (!result)
				return result;

			auto text = mono::from_mono_string(result);
			if (text.find("\n<color=#808080><i>") == std::string::npos)
				return result;

			static std::string s_cached_line;
			static bool s_label_initialized = false;
			if (!s_label_initialized)
			{
				auto localization = big::localization::get_instance();
				const auto label = localization.localize(std::string("$achievements_cheated_item_inventory"));
				if (!label.empty())
				{
					s_cached_line = "\n<color=#808080><i>" + label + "</i></color>";
					s_label_initialized = true;
				}
			}

			if (s_cached_line.empty())
				return result;

			auto pos = text.find(s_cached_line);
			if (pos == std::string::npos)
			{
				static bool logged_mismatch = false;
				if (!logged_mismatch)
				{
					logged_mismatch = true;
					LOG(WARNING) << "Item tooltip filter: localized cheat label did not match.";
				}
				return result;
			}

			do
			{
				text.erase(pos, s_cached_line.size());
				pos = text.find(s_cached_line, pos);
			} while (pos != std::string::npos);

			auto filtered = mono::to_mono_string(text);
			static bool logged_removal = false;
			if (filtered && !logged_removal)
			{
				logged_removal = true;
				LOG(INFO) << "Item tooltip filter removed the dev-command label.";
			}
			return filtered ? filtered : result;
		}
		EXCEPT_CLAUSE

		return detour_base::get_original<item_get_tooltip>()(item, quality, crafting, world_level, stack_override, appending);
	}
}
