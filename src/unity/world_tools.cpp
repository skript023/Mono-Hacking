#include "world_tools.hpp"

#include "item_data.hpp"
#include "item_drop.hpp"
#include "equipment_slots.hpp"
#include "localization.hpp"
#include <astra/host/notification.hpp>
#include "player.hpp"
#include "utility/unity.hpp"
#include "zone_system.hpp"
#include "task_mgr.hpp"

#include <format>
#include <unordered_set>


namespace big
{
	namespace
	{
		const std::vector<world_tools::raid_info> s_raids = {{"army_eikthyr", "Eikthyr rallies the creatures", "Boars & Necks"}, {"army_theelder", "The forest is moving...", "Greydwarfs, Brutes, Shamans"}, {"army_bonemass", "A foul smell from the swamp", "Draugr & Skeletons"}, {"army_moder", "A cold wind blows from the mountains", "Drakes"}, {"army_goblin", "The horde is attacking", "Fuling war party"}, {"army_queen", "They sought you out", "Seekers & Broods"}, {"wolves", "You are being hunted...", "Pack of aggressive Wolves"}, {"skeletons", "Skeleton Surprise", "Armored Skeletons"}, {"surtlings", "There's a smell of sulfur in the air", "Surtlings"}, {"bats", "You stirred the cauldron", "Cave Bats"}};

		MonoObject* get_player_profile()
		{
			auto game = unity::get_game();
			if (!game)
			{
				return nullptr;
			}

			static auto method = mono::get_method("Game", "GetPlayerProfile", 0, "assembly_valheim");
			if (!method)
			{
				return nullptr;
			}

			return mono::invoke_method(method, game, nullptr);
		}

		bool is_world_server()
		{
			auto znet             = mono::get_static_field_value("ZNet", "m_instance", "assembly_valheim");
			static auto is_server = mono::get_method("ZNet", "IsServer", 0, "assembly_valheim");
			if (!znet || !is_server)
			{
				return false;
			}
			auto result = mono::invoke_method(is_server, znet, nullptr);
			auto value  = result ? mono::object_unbox(result) : nullptr;
			return value && *reinterpret_cast<bool*>(value);
		}
	}

	std::vector<world_tools::raid_info> world_tools::get_available_raids_impl()
	{
		auto system        = unity::get_rand_event_system();
		auto system_class  = system ? mono::object_get_class(system) : nullptr;
		auto events_field  = system_class ? mono::get_field(system_class, "m_events") : nullptr;
		MonoObject* events = nullptr;
		if (system && events_field)
		{
			mono::get_field_value(system, events_field, &events);
		}
		if (!events)
		{
			return s_raids;
		}

		auto event_class   = mono::get_class("RandomEvent", "assembly_valheim");
		auto name_field    = event_class ? mono::get_field(event_class, "m_name") : nullptr;
		auto enabled_field = event_class ? mono::get_field(event_class, "m_enabled") : nullptr;
		auto message_field = event_class ? mono::get_field(event_class, "m_startMessage") : nullptr;
		auto spawn_field   = event_class ? mono::get_field(event_class, "m_spawn") : nullptr;
		if (!event_class || !name_field)
		{
			return s_raids;
		}

		std::vector<raid_info> result;
		std::unordered_set<std::string> seen;
		for (auto* event : unity::list_to_vector(events))
		{
			if (!event)
			{
				continue;
			}
			bool enabled = true;
			if (enabled_field)
			{
				mono::get_field_value(event, enabled_field, &enabled);
			}
			if (!enabled)
			{
				continue;
			}

			MonoString* name_string = nullptr;
			mono::get_field_value(event, name_field, &name_string);
			const auto internal_name = mono::from_mono_string(name_string);
			if (internal_name.empty() || !seen.insert(internal_name).second)
			{
				continue;
			}

			raid_info row;
			row.internal_name          = internal_name;
			MonoString* message_string = nullptr;
			if (message_field)
			{
				mono::get_field_value(event, message_field, &message_string);
			}
			row.display_name = message_string ? localization::get_instance().localize(mono::from_mono_string(message_string)) : internal_name;
			if (row.display_name.empty() || row.display_name == internal_name)
			{
				row.display_name = internal_name;
			}

			MonoObject* spawns = nullptr;
			if (spawn_field)
			{
				mono::get_field_value(event, spawn_field, &spawns);
			}
			if (spawns)
			{
				for (auto* spawn : unity::list_to_vector(spawns))
				{
					if (!spawn)
					{
						continue;
					}
					auto spawn_class         = mono::object_get_class(spawn);
					auto spawn_enabled_field = spawn_class ? mono::get_field(spawn_class, "m_enabled") : nullptr;
					bool spawn_enabled       = true;
					if (spawn_enabled_field)
					{
						mono::get_field_value(spawn, spawn_enabled_field, &spawn_enabled);
					}
					if (!spawn_enabled)
					{
						continue;
					}
					auto spawn_name_field         = spawn_class ? mono::get_field(spawn_class, "m_name") : nullptr;
					MonoString* spawn_name_string = nullptr;
					if (spawn_name_field)
					{
						mono::get_field_value(spawn, spawn_name_field, &spawn_name_string);
					}
					std::string spawn_name = mono::from_mono_string(spawn_name_string);
					if (spawn_name.empty())
					{
						auto prefab_field  = spawn_class ? mono::get_field(spawn_class, "m_prefab") : nullptr;
						MonoObject* prefab = nullptr;
						if (prefab_field)
						{
							mono::get_field_value(spawn, prefab_field, &prefab);
						}
						spawn_name = prefab ? unity::get_name(prefab) : std::string{};
					}
					if (!spawn_name.empty())
					{
						if (!row.enemies.empty())
						{
							row.enemies += ", ";
						}
						row.enemies += spawn_name;
					}
				}
			}
			if (row.enemies.empty())
			{
				row.enemies = "No configured spawns";
			}
			result.push_back(std::move(row));
		}
		std::sort(result.begin(), result.end(), [](const raid_info& left, const raid_info& right) {
			return left.display_name < right.display_name;
		});
		return result;
	}

	std::string world_tools::get_current_raid_name_impl()
	{
		auto res = unity::get_rand_event_system();
		if (!res)
		{
			return "Game world not loaded";
		}

		auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
		if (!res_class)
		{
			return "Unknown";
		}

		static auto f_cur_event = mono::get_field(res_class, "m_randomEvent");
		if (!f_cur_event)
		{
			return "None";
		}

		MonoObject* cur_ev = nullptr;
		mono::get_field_value(res, f_cur_event, &cur_ev);
		if (!cur_ev)
		{
			return "None (Base Safe)";
		}

		auto ev_class = mono::get_class("RandomEvent", "assembly_valheim");
		if (ev_class)
		{
			static auto f_name = mono::get_field(ev_class, "m_name");
			if (f_name)
			{
				MonoString* ms = nullptr;
				mono::get_field_value(cur_ev, f_name, &ms);
				if (ms)
				{
					return mono::from_mono_string(ms);
				}
			}
		}

		return "Active Raid Event";
	}

	void world_tools::trigger_raid_impl(const std::string& internal_name)
	{
		auto res    = unity::get_rand_event_system();
		auto player = unity::get_local_player();
		if (!res || !player)
		{
			notification::warning("Raid Controller", "Join a world first.");
			return;
		}
		if (!is_world_server())
		{
			notification::warning("Raid Controller", "Starting a selected raid requires hosting the world. The game has no client RPC for choosing a specific event.");
			return;
		}

		auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
		if (!res_class)
		{
			return;
		}

		static auto f_events = mono::get_field(res_class, "m_events");
		if (!f_events)
		{
			return;
		}

		MonoObject* list_obj = nullptr;
		mono::get_field_value(res, f_events, &list_obj);
		if (!list_obj)
		{
			return;
		}

		auto ev_list  = unity::list_to_vector(list_obj);
		auto ev_class = mono::get_class("RandomEvent", "assembly_valheim");
		auto f_name   = ev_class ? mono::get_field(ev_class, "m_name") : nullptr;

		MonoObject* target_ev = nullptr;
		for (auto* ev : ev_list)
		{
			if (!ev || !f_name)
			{
				continue;
			}

			MonoString* ms = nullptr;
			mono::get_field_value(ev, f_name, &ms);
			if (ms && mono::from_mono_string(ms) == internal_name)
			{
				target_ev = ev;
				break;
			}
		}

		if (target_ev)
		{
			static auto set_ev_method = mono::get_method("RandEventSystem", "SetRandomEvent", 2, "assembly_valheim");
			if (set_ev_method)
			{
				Vector3 pos           = unity::get_position(player);
				void* args[2]         = {target_ev, &pos};
				MonoObject* exception = nullptr;
				mono::invoke_method(set_ev_method, res, args, &exception);
				if (!exception)
				{
					notification::success("Raid Controller", std::format("Raid '{}' triggered!", internal_name));
					return;
				}
			}
		}

		notification::warning("Raid Controller", "Failed to start raid event.");
	}

	void world_tools::stop_current_raid_impl()
	{
		auto res = unity::get_rand_event_system();
		if (!res)
		{
			notification::warning("Raid Controller", "Join a world first.");
			return;
		}

		static auto reset_method = mono::get_method("RandEventSystem", "ConsoleResetRandomEvent", 0, "assembly_valheim");
		if (reset_method)
		{
			MonoObject* exception = nullptr;
			mono::invoke_method(reset_method, res, nullptr, &exception);
			if (!exception)
			{
				notification::success("Raid Controller", is_world_server() ? "Active raid stopped and dismissed!" : "Raid stop request sent to the host.");
				return;
			}
		}
		notification::warning("Raid Controller", "Failed to stop the active raid event.");
	}

	bool world_tools::has_death_point_impl()
	{
		auto prof = get_player_profile();
		if (!prof)
		{
			return false;
		}

		static auto method = mono::get_method("PlayerProfile", "HaveDeathPoint", 0, "assembly_valheim");
		if (!method)
		{
			return false;
		}

		auto ret = mono::invoke_method(method, prof, nullptr);
		if (!ret)
		{
			return false;
		}

		auto unboxed = mono::object_unbox(ret);
		return unboxed ? *reinterpret_cast<bool*>(unboxed) : false;
	}

	Vector3 world_tools::get_death_point_impl()
	{
		auto prof = get_player_profile();
		if (!prof)
		{
			return Vector3{0.f, 0.f, 0.f};
		}

		static auto method = mono::get_method("PlayerProfile", "GetDeathPoint", 0, "assembly_valheim");
		if (!method)
		{
			return Vector3{0.f, 0.f, 0.f};
		}

		auto ret = mono::invoke_method(method, prof, nullptr);
		if (!ret)
		{
			return Vector3{0.f, 0.f, 0.f};
		}

		auto unboxed = mono::object_unbox(ret);
		return unboxed ? *reinterpret_cast<Vector3*>(unboxed) : Vector3{0.f, 0.f, 0.f};
	}

	bool world_tools::teleport_to_tombstone_impl()
	{
		if (!has_death_point())
		{
			notification::warning("Tombstone", "No death point recorded on your player profile.");
			return false;
		}

		Vector3 dp = get_death_point();
		unity::teleport_to(dp + Vector3{0.f, 1.f, 0.f}, Vector4{0.f, 0.f, 0.f, 1.f}, true);
		notification::success("Tombstone", std::format("Teleported to last death location: {:.1f}, {:.1f}, {:.1f}", dp.x, dp.y, dp.z));
		return true;
	}

	bool world_tools::loot_nearby_tombstone_impl(float radius)
	{
		auto player_obj = unity::get_local_player();
		if (!player_obj)
		{
			return false;
		}

		static auto find_objects = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		static auto tombstone_class = mono::get_class("TombStone", "assembly_valheim");
		static auto container_class = mono::get_class("Container", "assembly_valheim");
		static auto get_component = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		static auto get_inventory = mono::get_method("Container", "GetInventory", 0, "assembly_valheim");
		static auto item_count    = mono::get_method("Inventory", "NrOfItems", 0, "assembly_valheim");
		static auto interact = mono::get_method_exact("TombStone", "Interact", {"Humanoid", "System.Boolean", "System.Boolean"}, "assembly_valheim");
		auto tombstone_type = tombstone_class ? mono::reflection_type(tombstone_class) : nullptr;
		auto container_type = container_class ? mono::reflection_type(container_class) : nullptr;
		if (!find_objects || !tombstone_type || !container_type || !get_component || !get_inventory || !item_count || !interact)
		{
			notification::warning("Tombstone", "Valheim's tombstone interaction methods are unavailable.");
			return false;
		}

		void* find_args[] = {tombstone_type};
		auto objects      = mono::invoke_method(find_objects, nullptr, find_args);
		if (!objects)
		{
			notification::warning("Tombstone", "Could not scan loaded tombstones.");
			return false;
		}

		const Vector3 player_pos = unity::get_position(player_obj);
		const float max_distance = std::clamp(radius, 1.f, 500.f);
		MonoObject* nearest      = nullptr;
		float nearest_distance   = max_distance;
		void* component_args[]   = {container_type};
		for (auto* tombstone : mono_array_view<MonoObject*>(reinterpret_cast<MonoArray*>(objects)))
		{
			if (!tombstone)
			{
				continue;
			}
			auto container = mono::invoke_method(get_component, tombstone, component_args);
			if (!container)
			{
				continue;
			}
			auto inventory = mono::invoke_method(get_inventory, container, nullptr);
			auto count     = inventory ? mono::invoke_method(item_count, inventory, nullptr) : nullptr;
			if (!count || *static_cast<int*>(mono::object_unbox(count)) <= 0)
			{
				continue;
			}
			const float distance = player_pos.distance(unity::get_position(tombstone));
			if (distance <= nearest_distance)
			{
				nearest          = tombstone;
				nearest_distance = distance;
			}
		}
		if (!nearest)
		{
			notification::info("Tombstone", std::format("No loaded tombstones with items found within {:.0f}m.", max_distance));
			return false;
		}

		bool hold             = false;
		bool alt              = false;
		void* interact_args[] = {player_obj, &hold, &alt};
		auto result           = mono::invoke_method(interact, nearest, interact_args);
		const bool interacted = result && *static_cast<bool*>(mono::object_unbox(result));
		if (interacted)
		{
			notification::success("Tombstone", std::format("Interacted with the nearest loaded grave ({:.0f}m).", nearest_distance));
		}
		else
		{
			notification::warning("Tombstone", "The nearest grave refused interaction. Check ownership, access, and inventory space.");
		}
		return interacted;
	}

	bool world_tools::open_trader_gui_impl(const std::string& trader_name)
	{
		auto store_gui = unity::get_store_gui();
		if (!store_gui)
		{
			notification::warning("Remote Trader", "StoreGui instance not available. Enter world first!");
			return false;
		}

		auto store_class = mono::get_class("StoreGui", "assembly_valheim");
		if (!store_class)
		{
			return false;
		}

		// Expand hide distance so it never auto-closes
		static auto f_hide_dist = mono::get_field(store_class, "m_hideDistance");
		static std::atomic_bool hide_distance_extended = false;
		if (f_hide_dist && !hide_distance_extended.exchange(true))
		{
			float original = 0.f;
			mono::get_field_value(store_gui, f_hide_dist, &original);
			float max_dist = 99999999.f;
			mono::set_field_value(store_gui, f_hide_dist, &max_dist);
			async_run([handle = mono::retain(store_gui), original]() -> task<void> {
				static auto is_visible = mono::get_method("StoreGui", "IsVisible", 0, "assembly_valheim");
				while (g_running && is_visible)
				{
					co_await this_task::wait();
					auto visible = mono::invoke_method(is_visible, nullptr, nullptr);
					if (!visible || !*static_cast<bool*>(mono::object_unbox(visible)))
					{
						break;
					}
				}
				if (auto gui = mono::retained_object(handle))
				{
					float value = original;
					mono::set_field_value(gui, f_hide_dist, &value);
				}
				mono::release(handle);
				hide_distance_extended = false;
			});
		}

		auto trader_class         = mono::get_class("Trader", "assembly_valheim");
		MonoObject* target_trader = nullptr;

		// 1. Check if any Trader is loaded in active scene
		static auto find_objs = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		if (find_objs && trader_class)
		{
			auto trader_type = mono::reflection_type(trader_class);
			void* fargs[1]   = {trader_type};
			auto arr         = reinterpret_cast<MonoArray*>(mono::invoke_method(find_objs, nullptr, fargs));
			if (arr && mono::array_length(arr) > 0)
			{
				for (int i = 0; i < mono::array_length(arr); ++i)
				{
					auto obj = *static_cast<MonoObject**>(mono::array_with_size(arr, sizeof(MonoObject*), i));
					if (!obj)
					{
						continue;
					}

					if (unity::get_name(obj).starts_with(trader_name))
					{
						target_trader = obj;
						break;
					}
				}
			}
		}

		// 2. Fallback: get Trader component from ZNetScene prefab
		if (!target_trader)
		{
			MonoObject* prefab = unity::get_znet_prefab(trader_name);
			if (prefab)
			{
				static auto comp_m = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				if (comp_m && trader_class)
				{
					auto t_type    = mono::reflection_type(trader_class);
					void* cargs[1] = {t_type};
					target_trader  = mono::invoke_method(comp_m, prefab, cargs);
				}
			}
		}

		if (!target_trader)
		{
			notification::warning("Remote Trader", std::format("Could not find trader '{}'.", trader_name));
			return false;
		}

		// Safe animator setup to prevent NullReferenceException on buy/sell
		if (trader_class)
		{
			static auto f_anim = mono::get_field(trader_class, "m_animator");
			static auto is_alive = mono::get_method_exact("Object", "op_Implicit", {"UnityEngine.Object"}, "UnityEngine.CoreModule", "UnityEngine");
			if (f_anim && is_alive)
			{
				MonoObject* cur_anim = nullptr;
				mono::get_field_value(target_trader, f_anim, &cur_anim);
				void* alive_args[1] = {cur_anim};
				auto alive          = cur_anim ? mono::invoke_method(is_alive, nullptr, alive_args) : nullptr;
				if (!alive || !*static_cast<bool*>(mono::object_unbox(alive)))
				{
					static auto comp_in_children = mono::get_method_exact("Component", "GetComponentInChildren", {"System.Type", "System.Boolean"}, "UnityEngine.CoreModule", "UnityEngine");
					auto anim_class = mono::get_class("Animator", "UnityEngine.AnimationModule", "UnityEngine");
					if (comp_in_children && anim_class)
					{
						auto anim_type          = mono::reflection_type(anim_class);
						bool include_inactive   = true;
						void* aargs[2]          = {anim_type, &include_inactive};
						MonoObject* trader_anim = mono::invoke_method(comp_in_children, target_trader, aargs);
						if (trader_anim)
						{
							mono::set_field_value(target_trader, f_anim, trader_anim);
						}
					}
				}
			}
		}

		static auto show_method = mono::get_method("StoreGui", "Show", 1, "assembly_valheim");
		if (show_method)
		{
			void* args[1] = {target_trader};
			mono::invoke_method(show_method, store_gui, args);
			notification::success("Remote Trader", std::format("Opened {} Store!", trader_name));
			return true;
		}

		return false;
	}

	bool world_tools::apply_world_preset_impl(const std::string& preset)
	{
		static auto preset_class = mono::get_class("WorldPresets", "assembly_valheim");
		static auto enum_parse = mono::get_method_exact("Enum", "Parse", {"System.Type", "System.String"}, "mscorlib", "System");
		static auto gui_class = mono::get_class("ServerOptionsGUI", "assembly_valheim");
		static auto read_keys = mono::get_method("ServerOptionsGUI", "ReadKeys", 1, "assembly_valheim");
		static auto set_preset = mono::get_method_exact("ServerOptionsGUI", "SetPreset", {"World", "WorldPresets"}, "assembly_valheim");
		if (!preset_class || !enum_parse || !gui_class || !read_keys || !set_preset)
		{
			return false;
		}

		auto znet_class  = mono::get_class("ZNet", "assembly_valheim");
		auto world_field = znet_class ? mono::get_field(znet_class, "m_world") : nullptr;
		auto znet_static = znet_class ? mono::get_static_field_data(znet_class) : nullptr;
		MonoObject* world = world_field && znet_static ? *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(znet_static) + mono::get_field_offset(world_field)) : nullptr;

		auto gui_field  = mono::get_field(gui_class, "m_instance");
		auto gui_static = mono::get_static_field_data(gui_class);
		MonoObject* gui = gui_field && gui_static ? *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(gui_static) + mono::get_field_offset(gui_field)) : nullptr;
		if (!world || !gui)
		{
			return false;
		}

		void* parse_args[2]   = {mono::reflection_type(preset_class), mono::to_mono_string(preset)};
		MonoObject* exception = nullptr;
		auto parsed_preset    = mono::invoke_method(enum_parse, nullptr, parse_args, &exception);
		if (exception || !parsed_preset || !mono::object_unbox(parsed_preset))
		{
			return false;
		}

		void* world_args[1] = {world};
		mono::invoke_method(read_keys, gui, world_args, &exception);
		if (exception)
		{
			return false;
		}
		void* preset_args[2] = {world, mono::object_unbox(parsed_preset)};
		mono::invoke_method(set_preset, gui, preset_args, &exception);
		return exception == nullptr;
	}

	bool world_tools::apply_world_modifier_impl(const std::string& modifier, const std::string& option)
	{
		static auto modifier_class = mono::get_class("WorldModifiers", "assembly_valheim");
		static auto option_class   = mono::get_class("WorldModifierOption", "assembly_valheim");
		static auto enum_parse = mono::get_method_exact("Enum", "Parse", {"System.Type", "System.String"}, "mscorlib", "System");
		static auto gui_class = mono::get_class("ServerOptionsGUI", "assembly_valheim");
		static auto read_keys = mono::get_method("ServerOptionsGUI", "ReadKeys", 1, "assembly_valheim");
		static auto set_modifier = mono::get_method_exact("ServerOptionsGUI", "SetPreset", {"World", "WorldModifiers", "WorldModifierOption"}, "assembly_valheim");
		if (!modifier_class || !option_class || !enum_parse || !gui_class || !read_keys || !set_modifier)
		{
			return false;
		}

		auto znet_class  = mono::get_class("ZNet", "assembly_valheim");
		auto world_field = znet_class ? mono::get_field(znet_class, "m_world") : nullptr;
		auto znet_static = znet_class ? mono::get_static_field_data(znet_class) : nullptr;
		MonoObject* world = world_field && znet_static ? *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(znet_static) + mono::get_field_offset(world_field)) : nullptr;
		auto gui_field  = mono::get_field(gui_class, "m_instance");
		auto gui_static = mono::get_static_field_data(gui_class);
		MonoObject* gui = gui_field && gui_static ? *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(gui_static) + mono::get_field_offset(gui_field)) : nullptr;
		if (!world || !gui)
		{
			return false;
		}

		void* modifier_args[] = {mono::reflection_type(modifier_class), mono::to_mono_string(modifier)};
		void* option_args[]   = {mono::reflection_type(option_class), mono::to_mono_string(option)};
		MonoObject* exception = nullptr;
		auto parsed_modifier  = mono::invoke_method(enum_parse, nullptr, modifier_args, &exception);
		if (exception || !parsed_modifier || !mono::object_unbox(parsed_modifier))
		{
			return false;
		}
		auto parsed_option = mono::invoke_method(enum_parse, nullptr, option_args, &exception);
		if (exception || !parsed_option || !mono::object_unbox(parsed_option))
		{
			return false;
		}

		void* world_args[] = {world};
		mono::invoke_method(read_keys, gui, world_args, &exception);
		if (exception)
		{
			return false;
		}
		if (auto zone = zone_system::get_instance(); zone.get_object())
		{
			static auto modifiers_field       = mono::get_field(gui_class, "m_modifiers");
			static auto slider_class          = mono::get_class("KeySlider", "assembly_valheim");
			static MonoClassField* keys_field = nullptr;
			auto modifiers = modifiers_field ? *reinterpret_cast<MonoArray**>(reinterpret_cast<uintptr_t>(gui_static) + mono::get_field_offset(modifiers_field)) : nullptr;
			const int target = *static_cast<int*>(mono::object_unbox(parsed_modifier));
			for (auto key_ui : mono_array_view<MonoObject*>(modifiers))
			{
				if (!key_ui || mono::object_get_class(key_ui) != slider_class || mono::get_field_value<"KeySlider", "m_modifier", int>(key_ui) != target)
				{
					continue;
				}
				for (auto setting : unity::list_to_vector(mono::get_field_value<"KeySlider", "m_settings", MonoObject*>(key_ui)))
				{
					if (!setting)
					{
						continue;
					}
					if (!keys_field)
					{
						keys_field = mono::get_field(mono::object_get_class(setting), "m_keys");
					}
					MonoObject* keys = nullptr;
					if (keys_field)
					{
						mono::get_field_value(setting, keys_field, &keys);
					}
					for (auto key : unity::list_to_vector(keys))
					{
						const auto name = mono::from_mono_string(reinterpret_cast<MonoString*>(key));
						zone.remove_global_key(name.substr(0, name.find(' ')));
					}
				}
			}
		}
		void* modifier_set_args[] = {world, mono::object_unbox(parsed_modifier), mono::object_unbox(parsed_option)};
		mono::invoke_method(set_modifier, gui, modifier_set_args, &exception);
		return exception == nullptr;
	}

	int world_tools::pickup_nearby_drops_impl(float radius)
	{
		auto player_obj = unity::get_local_player();
		if (!player_obj)
		{
			return 0;
		}

		static auto pickup = mono::get_method_exact("Humanoid", "Pickup", {"UnityEngine.GameObject", "System.Boolean", "System.Boolean"}, "assembly_valheim");
		if (!pickup)
		{
			return 0;
		}

		const Vector3 player_pos = unity::get_position(player_obj);
		const float max_distance = std::clamp(radius, 1.f, 100.f);
		std::vector<MonoObject*> candidates;
		for (auto drop : item_drop::get_drops())
		{
			if (drop && drop.get_object())
			{
				candidates.push_back(drop.get_object());
			}
		}

		int collected = 0;
		for (auto* drop_obj : candidates)
		{
			item_drop drop(drop_obj);
			if (!drop || !drop.can_pickup(false))
			{
				continue;
			}
			auto position = drop.get_position();
			if (!position || player_pos.distance(*position) > max_distance)
			{
				continue;
			}

			bool auto_equip   = false;
			bool pickup_delay = false;
			void* args[3]     = {drop_obj, &auto_equip, &pickup_delay};
			auto result       = mono::invoke_method(pickup, player_obj, args);
			if (result && *static_cast<bool*>(mono::object_unbox(result)))
			{
				++collected;
			}
		}

		return collected;
	}

	bool world_tools::repair_all_inventory_impl()
	{
		auto player_obj = unity::get_local_player();
		if (!player_obj)
		{
			notification::warning("Repair Tools", "Local player not loaded. Join world first!");
			return false;
		}

		player p(player_obj);
		auto inv     = p.get_inventory();
		auto inv_obj = inv.get_object();
		if (!inv_obj)
		{
			notification::warning("Repair Tools", "Inventory not available.");
			return false;
		}

		auto& equipment = equipment_manager::get();
		equipment.init_or_sync();
		auto equipment_inv        = equipment.get_equipment_inventory();
		static auto get_all_items = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
		if (!get_all_items)
		{
			return false;
		}

		auto items = unity::list_to_vector(mono::invoke_method(get_all_items, inv_obj, nullptr));
		if (equipment_inv && equipment_inv != inv_obj)
		{
			auto equipment_items = unity::list_to_vector(mono::invoke_method(get_all_items, equipment_inv, nullptr));
			for (auto* item : equipment_items)
			{
				if (item && std::find(items.begin(), items.end(), item) == items.end())
				{
					items.push_back(item);
				}
			}
		}
		items.erase(std::remove(items.begin(), items.end(), nullptr), items.end());
		int repaired_count = 0;
		for (auto item : items)
		{
			if (!item)
			{
				continue;
			}

			item_data id(item);
			float cur_dur = id.get_durability();
			float max_dur = id.get_max_durability();
			if (max_dur > 0.f && cur_dur < max_dur)
			{
				id.set_durability(max_dur);
				repaired_count++;
			}
		}

		if (repaired_count > 0)
		{
			static auto changed = []() {
				auto m = mono::get_method("Inventory", "Changed", 2, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method("Inventory", "Changed", 0, "assembly_valheim");
				}
				return m;
			}();
			if (changed)
			{
				if (mono::get_param_count(changed) == 2)
				{
					bool success  = true;
					bool cheated  = false;
					void* args[2] = {&success, &cheated};
					mono::invoke_method(changed, inv_obj, args);
					if (equipment_inv && equipment_inv != inv_obj)
					{
						mono::invoke_method(changed, equipment_inv, args);
					}
				}
				else
				{
					mono::invoke_method(changed, inv_obj, nullptr);
					if (equipment_inv && equipment_inv != inv_obj)
					{
						mono::invoke_method(changed, equipment_inv, nullptr);
					}
				}
			}
			notification::success("Repair Tools", std::format("Repaired {} inventory and equipped items!", repaired_count));
		}
		else
		{
			notification::info("Repair Tools", "All items are already in pristine condition!");
		}

		return true;
	}

	bool world_tools::generate_missing_locations_impl()
	{
		auto zs = zone_system::get_instance();
		if (!zs)
		{
			notification::warning("World Upgrade", "ZoneSystem not loaded. Enter world first!");
			return false;
		}

		static auto gen_m = mono::get_method("ZoneSystem", "GenerateLocations", 0, "assembly_valheim");
		if (!gen_m)
		{
			notification::error("World Upgrade", "GenerateLocations method not found.");
			return false;
		}

		mono::invoke_method(gen_m, zs.get_object(), nullptr);
		notification::success("World Upgrade", "Generating missing locations across unexplored zones (background task started)!");
		return true;
	}

	bool world_tools::upgrade_terrain_impl()
	{
		auto player = unity::get_local_player();
		if (!player)
		{
			notification::warning("World Upgrade", "Player not loaded. Join world first!");
			return false;
		}

		static auto up_terrain = mono::get_method("TerrainComp", "UpgradeTerrain", 0, "assembly_valheim");
		static auto up_alpha   = mono::get_method("Heightmap", "UpdateTerrainAlpha", 0, "assembly_valheim");

		if (up_terrain)
		{
			mono::invoke_method(up_terrain, nullptr, nullptr);
		}
		if (up_alpha)
		{
			mono::invoke_method(up_alpha, nullptr, nullptr);
		}

		notification::success("World Upgrade", "Optimized old terrain modifications in loaded area!");
		return true;
	}

	bool world_tools::upgrade_worldgen_version_impl()
	{
		auto wg_class = mono::get_class("WorldGenerator", "assembly_valheim");
		if (!wg_class)
		{
			return false;
		}

		static auto get_wg = mono::get_method("WorldGenerator", "get_instance", 0, "assembly_valheim");
		MonoObject* wg_obj = nullptr;
		if (get_wg)
		{
			wg_obj = mono::invoke_method(get_wg, nullptr, nullptr);
		}
		if (!wg_obj)
		{
			auto f_inst = mono::get_field(wg_class, "m_instance");
			if (f_inst)
			{
				void* sdata = mono::get_static_field_data(wg_class);
				if (sdata)
				{
					wg_obj = *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(sdata) + mono::get_field_offset(f_inst));
				}
			}
		}

		if (!wg_obj)
		{
			notification::warning("World Upgrade", "WorldGenerator instance not found.");
			return false;
		}

		static auto f_world   = mono::get_field(wg_class, "m_world");
		static auto f_version = mono::get_field(wg_class, "m_version");
		int target_version    = 2;

		if (f_version)
		{
			mono::set_field_value(wg_obj, f_version, &target_version);
		}

		if (f_world)
		{
			MonoObject* world_obj = nullptr;
			mono::get_field_value(wg_obj, f_world, &world_obj);
			if (world_obj)
			{
				auto world_class       = mono::object_get_class(world_obj);
				static auto f_wgen_ver = mono::get_field(world_class, "m_worldGenVersion");
				if (f_wgen_ver)
				{
					mono::set_field_value(world_obj, f_wgen_ver, &target_version);
				}
			}
		}

		static auto ver_setup = mono::get_method("WorldGenerator", "VersionSetup", 1, "assembly_valheim");
		if (ver_setup)
		{
			void* vargs[1] = {&target_version};
			mono::invoke_method(ver_setup, wg_obj, vargs);
		}

		notification::success("World Upgrade", "World generation version upgraded to v2 (Modern rules)!");
		return true;
	}

	bool world_tools::force_spawn_location_impl(const std::string& location_name)
	{
		auto zs     = zone_system::get_instance();
		auto player = unity::get_local_player();
		if (!zs || !player)
		{
			notification::warning("World Upgrade", "Game world not ready.");
			return false;
		}

		Vector3 player_pos = unity::get_position(player);
		Vector3 forward    = unity::get_forward(player);
		Vector3 target_pos = player_pos + forward * 10.f;

		static auto test_spawn = mono::get_method("ZoneSystem", "TestSpawnLocation", 3, "assembly_valheim");
		if (!test_spawn)
		{
			notification::error("World Upgrade", "TestSpawnLocation method not found.");
			return false;
		}

		auto ms           = mono::to_mono_string(location_name);
		bool disable_save = false;
		void* args[3]     = {ms, &target_pos, &disable_save};
		auto res          = mono::invoke_method(test_spawn, zs.get_object(), args);
		if (res && *static_cast<bool*>(mono::object_unbox(res)))
		{
			notification::success("World Upgrade", std::format("Spawned location '{}' nearby!", location_name));
			return true;
		}
		else
		{
			notification::warning("World Upgrade", std::format("Failed to spawn location '{}'. Check location name.", location_name));
			return false;
		}
	}
}
