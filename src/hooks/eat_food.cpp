#include "hooking.hpp"
#include "utility/unity.hpp"
#include "logger/exception_handler.hpp"
#include "unity/food.hpp"

namespace big
{
	bool hooks::player_can_eat(MonoObject* player, MonoObject* item, bool show_messages)
	{
		if (!player || !item || player != unity::get_local_player() || g_settings.self.max_food_slots <= 3)
		{
			return detour_base::get_original<player_can_eat>()(player, item, show_messages);
		}

		TRY_CLAUSE
		{
			// 1. Get m_foods list from player
			static auto foods_field = mono::get_field("Player", "m_foods", "assembly_valheim");
			static uint32_t foods_offset = foods_field ? mono::get_field_offset(foods_field) : 0;
			MonoObject* foods_list = foods_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)player + foods_offset) : nullptr;
			if (!foods_list)
				return detour_base::get_original<player_can_eat>()(player, item, show_messages);

			static auto size_field = mono::get_field(mono::object_get_class(foods_list), "_size");
			static uint32_t size_offset = size_field ? mono::get_field_offset(size_field) : 0;
			int count = size_offset ? *reinterpret_cast<int*>((uintptr_t)foods_list + size_offset) : 0;

			// If current food count is under 3, let vanilla CanEat handle it completely
			if (count < 3)
			{
				return detour_base::get_original<player_can_eat>()(player, item, show_messages);
			}

			// Get item's shared name and dropPrefab name
			static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
			static uint32_t shared_offset = shared_field ? mono::get_field_offset(shared_field) : 0;
			MonoObject* shared = shared_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item + shared_offset) : nullptr;

			static auto shared_name_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_name", "assembly_valheim");
			static uint32_t shared_name_offset = shared_name_field ? mono::get_field_offset(shared_name_field) : 0;
			MonoString* item_name_mono = (shared && shared_name_offset) ? *reinterpret_cast<MonoString**>((uintptr_t)shared + shared_name_offset) : nullptr;
			std::string item_shared_name = item_name_mono ? mono::from_mono_string(item_name_mono) : "";

			static auto drop_prefab_field = mono::get_field("ItemDrop/ItemData", "m_dropPrefab", "assembly_valheim");
			static uint32_t drop_prefab_offset = drop_prefab_field ? mono::get_field_offset(drop_prefab_field) : 0;
			MonoObject* drop_prefab = drop_prefab_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item + drop_prefab_offset) : nullptr;
			std::string item_prefab_name = "";
			if (drop_prefab)
			{
				static auto get_name = mono::get_method("Object", "get_name", 0, "UnityEngine.CoreModule", "UnityEngine");
				if (get_name)
				{
					MonoObject* name_obj = mono::invoke_method(get_name, drop_prefab, nullptr);
					if (name_obj)
						item_prefab_name = mono::from_mono_string(reinterpret_cast<MonoString*>(name_obj));
				}
			}

			auto food_views = mono::list<food>(foods_list);

			// 2. Check if player already ate this exact food
			for (int i = 0; i < food_views.size(); i++)
			{
				auto f = food_views[i];
				bool match = false;
				if (!item_shared_name.empty() && f.get_shared_name() == item_shared_name)
					match = true;
				else if (!item_prefab_name.empty() && f.get_name() == item_prefab_name)
					match = true;

				if (match)
				{
					if (f.can_eat_again())
						return true;

					if (show_messages)
					{
						static auto msg_method = mono::get_method("Character", "Message", 2, "assembly_valheim");
						if (msg_method)
						{
							MonoString* msg = mono::to_mono_string("$msg_nomore");
							int msg_type = 2; // MessageType.Center
							void* params[2] = { &msg_type, msg };
							mono::invoke_method(msg_method, player, params);
						}
					}
					return false;
				}
			}

			// 3. Check if any existing food can be eaten again (can replace most depleted)
			for (int i = 0; i < food_views.size(); i++)
			{
				if (food_views[i].can_eat_again())
					return true;
			}

			// 4. Check against custom max_food_slots
			if (count >= g_settings.self.max_food_slots)
			{
				if (show_messages)
				{
					static auto msg_method = mono::get_method("Character", "Message", 2, "assembly_valheim");
					if (msg_method)
					{
						MonoString* msg = mono::to_mono_string("$msg_isfull");
						int msg_type = 2; // MessageType.Center
						void* params[2] = { &msg_type, msg };
						mono::invoke_method(msg_method, player, params);
					}
				}
				return false;
			}

			return true;
		} EXCEPT_CLAUSE

		return detour_base::get_original<player_can_eat>()(player, item, show_messages);
	}

	bool hooks::player_eat_food(MonoObject* player, MonoObject* item)
	{
		if (!player || !item || player != unity::get_local_player() || g_settings.self.max_food_slots <= 3)
		{
			return detour_base::get_original<player_eat_food>()(player, item);
		}

		TRY_CLAUSE
		{
			// 1. Get m_foods list
			static auto foods_field = mono::get_field("Player", "m_foods", "assembly_valheim");
			static uint32_t foods_offset = foods_field ? mono::get_field_offset(foods_field) : 0;
			MonoObject* foods_list = foods_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)player + foods_offset) : nullptr;
			if (!foods_list)
				return detour_base::get_original<player_eat_food>()(player, item);

			static auto size_field = mono::get_field(mono::object_get_class(foods_list), "_size");
			static uint32_t size_offset = size_field ? mono::get_field_offset(size_field) : 0;
			int count = size_offset ? *reinterpret_cast<int*>((uintptr_t)foods_list + size_offset) : 0;

			// If count < 3 or count >= max_food_slots, let vanilla EatFood handle it (it either adds or replaces most depleted)
			if (count < 3 || count >= g_settings.self.max_food_slots)
			{
				return detour_base::get_original<player_eat_food>()(player, item);
			}

			// Validate with can_eat (without showing messages)
			if (!hooks::player_can_eat(player, item, false))
			{
				return false;
			}

			// Get item's shared name and dropPrefab name
			static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
			static uint32_t shared_offset = shared_field ? mono::get_field_offset(shared_field) : 0;
			MonoObject* shared = shared_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item + shared_offset) : nullptr;
			if (!shared)
				return detour_base::get_original<player_eat_food>()(player, item);

			static auto shared_name_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_name", "assembly_valheim");
			static uint32_t shared_name_offset = shared_name_field ? mono::get_field_offset(shared_name_field) : 0;
			MonoString* item_name_mono = shared_name_offset ? *reinterpret_cast<MonoString**>((uintptr_t)shared + shared_name_offset) : nullptr;
			std::string item_shared_name = item_name_mono ? mono::from_mono_string(item_name_mono) : "";

			static auto drop_prefab_field = mono::get_field("ItemDrop/ItemData", "m_dropPrefab", "assembly_valheim");
			static uint32_t drop_prefab_offset = drop_prefab_field ? mono::get_field_offset(drop_prefab_field) : 0;
			MonoObject* drop_prefab = drop_prefab_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item + drop_prefab_offset) : nullptr;
			std::string item_prefab_name = "";
			MonoString* prefab_name_mono = nullptr;
			if (drop_prefab)
			{
				static auto get_name = mono::get_method("Object", "get_name", 0, "UnityEngine.CoreModule", "UnityEngine");
				if (get_name)
				{
					MonoObject* name_obj = mono::invoke_method(get_name, drop_prefab, nullptr);
					if (name_obj)
					{
						prefab_name_mono = reinterpret_cast<MonoString*>(name_obj);
						item_prefab_name = mono::from_mono_string(prefab_name_mono);
					}
				}
			}

			// Check if already in m_foods (refresh timer/stats)
			auto food_views = mono::list<food>(foods_list);
			for (int i = 0; i < food_views.size(); i++)
			{
				auto f = food_views[i];
				bool match = false;
				if (!item_shared_name.empty() && f.get_shared_name() == item_shared_name)
					match = true;
				else if (!item_prefab_name.empty() && f.get_name() == item_prefab_name)
					match = true;

				if (match)
				{
					// Already present in list, let vanilla refresh it
					return detour_base::get_original<player_eat_food>()(player, item);
				}
			}

			// We have 3 or more foods and room for more! Allocate and add new Food:
			static auto food_klass = mono::get_class("Player/Food", "assembly_valheim");
			if (!food_klass)
				return detour_base::get_original<player_eat_food>()(player, item);

			MonoObject* new_food = mono::object_new(food_klass);
			if (!new_food)
				return detour_base::get_original<player_eat_food>()(player, item);

			static auto food_ctor = mono::class_get_method_from_name(food_klass, ".ctor", 0);
			if (food_ctor)
			{
				mono::invoke_method(food_ctor, new_food, nullptr);
			}

			// Set m_name = item.m_dropPrefab.name
			if (prefab_name_mono)
			{
				mono::set_field_value<"Player/Food", "m_name">(new_food, prefab_name_mono);
			}

			// Set m_item = item
			mono::set_field_value<"Player/Food", "m_item">(new_food, item);

			// Read shared data
			static auto burn_time_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_foodBurnTime", "assembly_valheim");
			static auto food_hp_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_food", "assembly_valheim");
			static auto food_stam_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_foodStamina", "assembly_valheim");
			static auto food_eitr_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_foodEitr", "assembly_valheim");

			float burn_time = burn_time_field ? *reinterpret_cast<float*>((uintptr_t)shared + mono::get_field_offset(burn_time_field)) : 0.f;
			float hp = food_hp_field ? *reinterpret_cast<float*>((uintptr_t)shared + mono::get_field_offset(food_hp_field)) : 0.f;
			float stam = food_stam_field ? *reinterpret_cast<float*>((uintptr_t)shared + mono::get_field_offset(food_stam_field)) : 0.f;
			float eitr = food_eitr_field ? *reinterpret_cast<float*>((uintptr_t)shared + mono::get_field_offset(food_eitr_field)) : 0.f;

			mono::set_field_value<"Player/Food", "m_time">(new_food, burn_time);
			mono::set_field_value<"Player/Food", "m_health">(new_food, hp);
			mono::set_field_value<"Player/Food", "m_stamina">(new_food, stam);
			mono::set_field_value<"Player/Food", "m_eitr">(new_food, eitr);

			// Add to m_foods list
			auto list_class = mono::object_get_class(foods_list);
			static auto add_method = mono::class_get_method_from_name(list_class, "Add", 1);
			if (add_method)
			{
				void* params[1] = { new_food };
				mono::invoke_method(add_method, foods_list, params);
			}

			// Update food stats
			static auto update_food = mono::get_method("Player", "UpdateFood", 2, "assembly_valheim");
			if (update_food)
			{
				float dt = 0.f;
				bool force = true;
				void* uf_args[2] = { &dt, &force };
				mono::invoke_method(update_food, player, uf_args);
			}

			// Show message
			std::string text = "";
			if (hp > 0.f) text += " +" + std::to_string((int)hp) + " $item_food_health ";
			if (stam > 0.f) text += " +" + std::to_string((int)stam) + " $item_food_stamina ";
			if (eitr > 0.f) text += " +" + std::to_string((int)eitr) + " $item_food_eitr ";

			static auto msg_method = mono::get_method("Character", "Message", 2, "assembly_valheim");
			if (msg_method && !text.empty())
			{
				int msg_type = 2; // MessageType.Center
				MonoString* str = mono::to_mono_string(text);
				void* m_args[2] = { &msg_type, str };
				mono::invoke_method(msg_method, player, m_args);
			}

			return true;
		} EXCEPT_CLAUSE

		return detour_base::get_original<player_eat_food>()(player, item);
	}
}

