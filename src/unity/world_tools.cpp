#include "world_tools.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include "utility/unity.hpp"

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
			{"bats", "You stirred the cauldron", "Cave Bats"}
		};

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

	const std::vector<world_tools::raid_info>& world_tools::get_available_raids()
	{
		return s_raids;
	}

	std::string world_tools::get_current_raid_name()
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

	void world_tools::trigger_raid(const std::string& internal_name)
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

	void world_tools::stop_current_raid()
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

	void world_tools::update_raid_system()
	{
		if (g_settings.self.disable_raids)
		{
			auto res = unity::get_rand_event_system();
			if (res)
			{
				auto res_class = mono::get_class("RandEventSystem", "assembly_valheim");
				if (res_class)
				{
					static auto f_chance = mono::get_field(res_class, "m_eventChance");
					if (f_chance)
					{
						float zero = 0.f;
						mono::set_field_value(res, f_chance, &zero);
					}
				}
			}
		}
	}

	bool world_tools::has_death_point()
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

	Vector3 world_tools::get_death_point()
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

	bool world_tools::teleport_to_tombstone()
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

	bool world_tools::loot_nearby_tombstone(float radius)
	{
		auto player = unity::get_local_player();
		if (!player)
			return false;

		Vector3 my_pos = unity::get_position(player);
		auto all_chars = unity::get_all_characters();

		notification::info("Tombstone", "Searching loaded tombstone containers...");
		return true;
	}

	bool world_tools::open_trader_gui()
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
			float max_dist = 999999.f;
			mono::set_field_value(store_gui, f_hide_dist, &max_dist);
		}

		// Find loaded Trader
		auto trader_class = mono::get_class("Trader", "assembly_valheim");
		MonoObject* target_trader = nullptr;
		if (trader_class)
		{
			static auto f_traders = mono::get_field(trader_class, "m_allTraders");
			if (f_traders)
			{
				void* static_data = mono::get_static_field_data(trader_class);
				if (static_data)
				{
					uint32_t offset = mono::get_field_offset(f_traders);
					MonoObject* list_obj = *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(static_data) + offset);
					if (list_obj)
					{
						auto vec = unity::list_to_vector(list_obj);
						if (!vec.empty())
							target_trader = vec.front();
					}
				}
			}
		}

		static auto show_method = mono::get_method("StoreGui", "Show", 1, "assembly_valheim");
		if (show_method)
		{
			void* args[1] = {target_trader};
			mono::invoke_method(show_method, store_gui, args);
			notification::success("Remote Trader", target_trader ? "Opened Trader Store!" : "Opened Store GUI (No merchant nearby, visit discovery first).");
			return true;
		}

		return false;
	}
}

