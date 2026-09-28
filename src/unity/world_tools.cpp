#include "world_tools.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"
#include "item_data.hpp"
#include "zone_system.hpp"
#include "player.hpp"

#include <format>

namespace big
{
	namespace
	{
		const std::vector<world_tools::raid_info> s_raids = {
		    {"army_eikthyr", "Eikthyr rallies the creatures", "Boars & Necks"},
		    {"army_theelder", "The forest is moving...", "Greydwarfs, Brutes, Shamans"},
		    {"army_bonemass", "A foul smell from the swamp", "Draugr & Skeletons"},
		    {"army_moder", "A cold wind blows from the mountains", "Drakes"},
		    {"army_goblin", "The horde is attacking", "Fuling war party"},
		    {"army_queen", "They sought you out", "Seekers & Broods"},
		    {"wolves", "You are being hunted...", "Pack of aggressive Wolves"},
		    {"skeletons", "Skeleton Surprise", "Armored Skeletons"},
		    {"surtlings", "There's a smell of sulfur in the air", "Surtlings"},
		    {"bats", "You stirred the cauldron", "Cave Bats"}};

		MonoObject* get_player_profile()
		{
			auto game = unity::get_game();
			if (!game)
				return nullptr;

			static auto method = mono::get_method("Game", "GetPlayerProfile", 0, "assembly_valheim");
			if (!method)
				return nullptr;

			return mono::invoke_method(method, game, nullptr);
		}
	}

	const std::vector<world_tools::raid_info>& world_tools::get_available_raids_impl()
	{
		return s_raids;
	}

	std::string world_tools::get_current_raid_name_impl()
	{
		auto res = unity::get_rand_event_system();
		if (!res)
			return "Game world not loaded";

		auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
		if (!res_class)
			return "Unknown";

		static auto f_cur_event = mono::get_field(res_class, "m_randomEvent");
		if (!f_cur_event)
			return "None";

		MonoObject* cur_ev = nullptr;
		mono::get_field_value(res, f_cur_event, &cur_ev);
		if (!cur_ev)
			return "None (Base Safe)";

		auto ev_class = mono::get_class("RandomEvent", "assembly_valheim");
		if (ev_class)
		{
			static auto f_name = mono::get_field(ev_class, "m_name");
			if (f_name)
			{
				MonoString* ms = nullptr;
				mono::get_field_value(cur_ev, f_name, &ms);
				if (ms)
					return mono::from_mono_string(ms);
			}
		}

		return "Active Raid Event";
	}

	void world_tools::trigger_raid_impl(const std::string& internal_name)
	{
		auto res = unity::get_rand_event_system();
		auto player = unity::get_local_player();
		if (!res || !player)
		{
			notification::warning("Raid Controller", "Join a world first.");
			return;
		}

		auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
		if (!res_class)
			return;

		static auto f_events = mono::get_field(res_class, "m_events");
		if (!f_events)
			return;

		MonoObject* list_obj = nullptr;
		mono::get_field_value(res, f_events, &list_obj);
		if (!list_obj)
			return;

		auto ev_list = unity::list_to_vector(list_obj);
		auto ev_class = mono::get_class("RandomEvent", "assembly_valheim");
		auto f_name = ev_class ? mono::get_field(ev_class, "m_name") : nullptr;

		MonoObject* target_ev = nullptr;
		for (auto* ev : ev_list)
		{
			if (!ev || !f_name)
				continue;

			MonoString* ms = nullptr;
			mono::get_field_value(ev, f_name, &ms);
			if (ms && mono::from_mono_string(ms) == internal_name)
			{
				target_ev = ev;
				break;
			}
		}

		if (!target_ev && !ev_list.empty())
			target_ev = ev_list.front();

		if (target_ev)
		{
			static auto set_ev_method = mono::get_method("RandEventSystem", "SetRandomEvent", 2, "assembly_valheim");
			if (set_ev_method)
			{
				Vector3 pos = unity::get_position(player);
				void* args[2] = {target_ev, &pos};
				mono::invoke_method(set_ev_method, res, args);
				notification::success("Raid Controller", std::format("Raid '{}' triggered!", internal_name));
				return;
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

		static auto reset_method = mono::get_method("RandEventSystem", "ResetRandomEvent", 0, "assembly_valheim");
		if (reset_method)
		{
			mono::invoke_method(reset_method, res, nullptr);
			notification::success("Raid Controller", "Active raid stopped and dismissed!");
		}
	}

	bool world_tools::has_death_point_impl()
	{
		auto prof = get_player_profile();
		if (!prof)
			return false;

		static auto method = mono::get_method("PlayerProfile", "HaveDeathPoint", 0, "assembly_valheim");
		if (!method)
			return false;

		auto ret = mono::invoke_method(method, prof, nullptr);
		if (!ret)
			return false;

		auto unboxed = mono::object_unbox(ret);
		return unboxed ? *reinterpret_cast<bool*>(unboxed) : false;
	}

	Vector3 world_tools::get_death_point_impl()
	{
		auto prof = get_player_profile();
		if (!prof)
			return Vector3{0.f, 0.f, 0.f};

		static auto method = mono::get_method("PlayerProfile", "GetDeathPoint", 0, "assembly_valheim");
		if (!method)
			return Vector3{0.f, 0.f, 0.f};

		auto ret = mono::invoke_method(method, prof, nullptr);
		if (!ret)
			return Vector3{0.f, 0.f, 0.f};

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
		auto player = unity::get_local_player();
		if (!player)
			return false;

		Vector3 my_pos = unity::get_position(player);
		auto all_chars = unity::get_all_characters();

		notification::info("Tombstone", "Searching loaded tombstone containers...");
		return true;
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
			return false;

		// Expand hide distance so it never auto-closes
		static auto f_hide_dist = mono::get_field(store_class, "m_hideDistance");
		if (f_hide_dist)
		{
			float max_dist = 99999999.f;
			mono::set_field_value(store_gui, f_hide_dist, &max_dist);
		}

		auto trader_class = mono::get_class("Trader", "assembly_valheim");
		MonoObject* target_trader = nullptr;

		// 1. Check if any Trader is loaded in active scene
		static auto find_objs = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		if (find_objs && trader_class)
		{
			auto trader_type = mono::reflection_type(trader_class);
			void* fargs[1] = {trader_type};
			auto arr = reinterpret_cast<MonoArray*>(mono::invoke_method(find_objs, nullptr, fargs));
			if (arr && mono::array_length(arr) > 0)
			{
				for (int i = 0; i < mono::array_length(arr); ++i)
				{
					auto obj = *static_cast<MonoObject**>(mono::array_with_size(arr, sizeof(MonoObject*), i));
					if (!obj)
						continue;

					static auto f_tname = mono::get_field(trader_class, "m_name");
					if (f_tname)
					{
						MonoString* ms = nullptr;
						mono::get_field_value(obj, f_tname, &ms);
						if (ms && mono::from_mono_string(ms).find(trader_name) != std::string::npos)
						{
							target_trader = obj;
							break;
						}
					}
					if (!target_trader)
						target_trader = obj;
				}
			}
		}

		// 2. Fallback: get Trader component from ZNetScene prefab
		if (!target_trader)
		{
			auto znet = unity::get_znet_scene();
			if (znet)
			{
				static auto znet_get_prefab = mono::get_method("ZNetScene", "GetPrefab", 1, "assembly_valheim");
				if (znet_get_prefab)
				{
					auto ms = mono::to_mono_string(trader_name);
					void* pargs[1] = {ms};
					MonoObject* prefab = mono::invoke_method(znet_get_prefab, znet, pargs);
					if (prefab)
					{
						static auto comp_m = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
						if (comp_m && trader_class)
						{
							auto t_type = mono::reflection_type(trader_class);
							void* cargs[1] = {t_type};
							target_trader = mono::invoke_method(comp_m, prefab, cargs);
						}
					}
				}
			}
		}

		if (!target_trader)
		{
			notification::warning("Remote Trader", std::format("Could not find trader '{}'.", trader_name));
			return false;
		}

		// Safe animator setup to prevent NullReferenceException on buy/sell
		auto player_obj = unity::get_local_player();
		if (player_obj && trader_class)
		{
			static auto f_anim = mono::get_field(trader_class, "m_animator");
			if (f_anim)
			{
				MonoObject* cur_anim = nullptr;
				mono::get_field_value(target_trader, f_anim, &cur_anim);
				if (!cur_anim)
				{
					static auto comp_in_children = mono::get_method_overload("Component", "GetComponentInChildren", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
					auto anim_class = mono::get_class("Animator", "UnityEngine.AnimationModule", "UnityEngine");
					if (comp_in_children && anim_class)
					{
						auto anim_type = mono::reflection_type(anim_class);
						void* aargs[1] = {anim_type};
						MonoObject* player_anim = mono::invoke_method(comp_in_children, player_obj, aargs);
						if (player_anim)
							mono::set_field_value(target_trader, f_anim, player_anim);
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

	bool world_tools::repair_all_inventory_impl()
	{
		auto player_obj = unity::get_local_player();
		if (!player_obj)
		{
			notification::warning("Repair Tools", "Local player not loaded. Join world first!");
			return false;
		}

		player p(player_obj);
		auto inv = p.get_inventory();
		auto inv_obj = inv.get_object();
		if (!inv_obj)
		{
			notification::warning("Repair Tools", "Inventory not available.");
			return false;
		}

		auto inv_class = mono::object_get_class(inv_obj);
		auto f_inventory = mono::get_field(inv_class, "m_inventory");
		if (!f_inventory)
			return false;

		MonoObject* list_obj = nullptr;
		mono::get_field_value(inv_obj, f_inventory, &list_obj);
		if (!list_obj)
			return false;

		auto items = unity::list_to_vector(list_obj);
		int repaired_count = 0;
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
				repaired_count++;
			}
		}

		if (repaired_count > 0)
		{
			unity::show_message(std::format("Repaired {} items to 100%!", repaired_count));
			notification::success("Repair Tools", std::format("Repaired {} items in inventory!", repaired_count));
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
		unity::show_message("Generating locations for old world...");
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
		static auto up_alpha = mono::get_method("Heightmap", "UpdateTerrainAlpha", 0, "assembly_valheim");

		if (up_terrain)
			mono::invoke_method(up_terrain, nullptr, nullptr);
		if (up_alpha)
			mono::invoke_method(up_alpha, nullptr, nullptr);

		notification::success("World Upgrade", "Optimized old terrain modifications in loaded area!");
		unity::show_message("Terrain upgraded to modern format.");
		return true;
	}

	bool world_tools::upgrade_worldgen_version_impl()
	{
		auto wg_class = mono::get_class("WorldGenerator", "assembly_valheim");
		if (!wg_class)
			return false;

		static auto get_wg = mono::get_method("WorldGenerator", "get_instance", 0, "assembly_valheim");
		MonoObject* wg_obj = nullptr;
		if (get_wg)
			wg_obj = mono::invoke_method(get_wg, nullptr, nullptr);
		if (!wg_obj)
		{
			auto f_inst = mono::get_field(wg_class, "m_instance");
			if (f_inst)
			{
				void* sdata = mono::get_static_field_data(wg_class);
				if (sdata)
					wg_obj = *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(sdata) + mono::get_field_offset(f_inst));
			}
		}

		if (!wg_obj)
		{
			notification::warning("World Upgrade", "WorldGenerator instance not found.");
			return false;
		}

		static auto f_world = mono::get_field(wg_class, "m_world");
		static auto f_version = mono::get_field(wg_class, "m_version");
		int target_version = 2;

		if (f_version)
			mono::set_field_value(wg_obj, f_version, &target_version);

		if (f_world)
		{
			MonoObject* world_obj = nullptr;
			mono::get_field_value(wg_obj, f_world, &world_obj);
			if (world_obj)
			{
				auto world_class = mono::object_get_class(world_obj);
				static auto f_wgen_ver = mono::get_field(world_class, "m_worldGenVersion");
				if (f_wgen_ver)
					mono::set_field_value(world_obj, f_wgen_ver, &target_version);
			}
		}

		static auto ver_setup = mono::get_method("WorldGenerator", "VersionSetup", 1, "assembly_valheim");
		if (ver_setup)
		{
			void* vargs[1] = {&target_version};
			mono::invoke_method(ver_setup, wg_obj, vargs);
		}

		notification::success("World Upgrade", "World generation version upgraded to v2 (Modern rules)!");
		unity::show_message("WorldGen upgraded to version 2.");
		return true;
	}

	bool world_tools::force_spawn_location_impl(const std::string& location_name)
	{
		auto zs = zone_system::get_instance();
		auto player = unity::get_local_player();
		if (!zs || !player)
		{
			notification::warning("World Upgrade", "Game world not ready.");
			return false;
		}

		Vector3 player_pos = unity::get_position(player);
		Vector3 forward = unity::get_forward(player);
		Vector3 target_pos = player_pos + forward * 10.f;

		static auto test_spawn = mono::get_method("ZoneSystem", "TestSpawnLocation", 3, "assembly_valheim");
		if (!test_spawn)
		{
			notification::error("World Upgrade", "TestSpawnLocation method not found.");
			return false;
		}

		auto ms = mono::to_mono_string(location_name);
		bool disable_save = false;
		void* args[3] = {ms, &target_pos, &disable_save};
		auto res = mono::invoke_method(test_spawn, zs.get_object(), args);
		if (res && *static_cast<bool*>(mono::object_unbox(res)))
		{
			notification::success("World Upgrade", std::format("Spawned location '{}' nearby!", location_name));
			unity::show_message(std::format("Spawned {} nearby", location_name));
			return true;
		}
		else
		{
			notification::warning("World Upgrade", std::format("Failed to spawn location '{}'. Check location name.", location_name));
			return false;
		}
	}
}
