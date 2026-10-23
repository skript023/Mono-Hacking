#include "hooking.hpp"
#include "unity/item_drop.hpp"
#include "unity/localization.hpp"

namespace big
{
	static MonoObject* alive_item_drop(MonoObject* fish)
	{
		static auto is_alive = mono::get_method_exact("Object", "op_Implicit", {"UnityEngine.Object"}, "UnityEngine.CoreModule", "UnityEngine");
		auto item_drop = mono::get_field_value<"Fish", "m_itemDrop", MonoObject*>(fish);
		if (!item_drop || !is_alive)
		{
			return nullptr;
		}
		void* args[1] = {item_drop};
		auto alive    = mono::invoke_method(is_alive, nullptr, args);
		return alive && *static_cast<bool*>(mono::object_unbox(alive)) ? item_drop : nullptr;
	}

	bool hooks::fish_interact(MonoObject* fish, MonoObject* user, bool hold, bool alt)
	{
		if (g_running && g_settings.self.allow_pickup_fish && fish && !hold)
		{
			static auto pickup = mono::get_method("Fish", "Pickup", 1, "assembly_valheim");
			if (pickup)
			{
				void* args[1] = {user};
				auto picked   = mono::invoke_method(pickup, fish, args);
				return picked && *static_cast<bool*>(mono::object_unbox(picked));
			}
		}

		return detour_base::get_original<fish_interact>()(fish, user, hold, alt);
	}

	MonoString* hooks::fish_get_hover_text(MonoObject* fish)
	{
		if (g_running && g_settings.self.allow_pickup_fish && fish)
		{
			if (auto item_drop = alive_item_drop(fish))
			{
				return mono::to_mono_string(big::item_drop(item_drop).get_hover_text());
			}
			auto name = mono::get_field_value<"Fish", "m_name", MonoString*>(fish);
			auto text = mono::from_mono_string(name) + "\n[<color=yellow><b>$KEY_Use</b></color>] $inventory_pickup";
			return mono::to_mono_string(localization::get_instance().localize(text));
		}

		return detour_base::get_original<fish_get_hover_text>()(fish);
	}
}
