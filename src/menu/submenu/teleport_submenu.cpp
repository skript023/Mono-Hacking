#include "../view.hpp"
#include "../submenu.hpp"
#include "fiber_pool.hpp"
#include "utility/unity.hpp"
#include "unity/self.hpp"
#include "unity/localization.hpp"
#include "notification/notification_service.hpp"
#include "menu_settings.hpp"
#include "input/input_service.hpp"
#include <custom_teleport/custom_teleport_service.hpp>
#include <unordered_set>
#include <mutex>

namespace big
{
	std::string category = "Default";

	static std::unordered_map<std::string, std::string> s_localized_pin_cache;
	static std::unordered_set<std::string> s_requested_pins;
	static std::mutex s_pin_cache_mutex;

	static void queue_pin_localization(const std::string& token)
	{
		{
			std::lock_guard lock(s_pin_cache_mutex);
			if (s_requested_pins.contains(token))
				return;
			s_requested_pins.insert(token);
		}

		g_fiber_pool->queue_job([token] {
			auto loc = localization::get_instance().localize(token);
			if (!loc.empty() && loc != token && !(loc.front() == '[' && loc.back() == ']'))
			{
				std::lock_guard lock(s_pin_cache_mutex);
				s_localized_pin_cache[token] = loc;
			}
		});
	}

	static std::string resolve_pin_display_name(const std::string& raw_name, const std::string& default_prefix, int count)
	{
		if (raw_name.empty())
			return default_prefix + " #" + std::to_string(count);

		// 1. Thread-safe cached translation from Valheim Localization engine (resolved on game thread)
		{
			std::lock_guard lock(s_pin_cache_mutex);
			if (auto it = s_localized_pin_cache.find(raw_name); it != s_localized_pin_cache.end())
				return it->second;
		}

		// Queue localization job to game thread (fiber pool)
		if (raw_name.starts_with("$"))
		{
			queue_pin_localization(raw_name);
		}
		else
		{
			std::string token = "$" + raw_name;
			{
				std::lock_guard lock(s_pin_cache_mutex);
				if (auto it = s_localized_pin_cache.find(token); it != s_localized_pin_cache.end())
					return it->second;
			}
			queue_pin_localization(token);
		}

		// 2. Fallback: Known Valheim Bosses & Traders table
		static const std::unordered_map<std::string, std::string> known_names = {
		    // Bosses
		    {"$enemy_eikthyr", "Eikthyr"},
		    {"$enemy_gdking", "The Elder"},
		    {"$enemy_bonemass", "Bonemass"},
		    {"$enemy_dragon", "Moder"},
		    {"$enemy_goblinking", "Yagluth"},
		    {"$enemy_seekerqueen", "The Queen"},
		    {"$enemy_fader", "Fader"},

		    // Traders & Special Locations
		    {"$location_forestcrypt", "Haldor (Trader)"},
		    {"$location_hildir", "Hildir (Trader)"},
		    {"$location_bogwitch", "The Bog Witch (Trader)"},
		    {"$item_bed", "Bed / Spawn"},

		    // Dungeons & Other Locations
		    {"$location_crypt", "Sunken Crypt"},
		    {"$location_cave", "Mountain Cave"},
		    {"$location_dungeon", "Dungeon"},
		    {"$location_mountaincave", "Frost Caves"}};

		if (auto it = known_names.find(raw_name); it != known_names.end())
			return it->second;

		// 3. Fallback: Clean string formatting (strip prefix and Title Case)
		if (raw_name.starts_with("$"))
		{
			std::string stripped = raw_name.substr(1);
			if (stripped.starts_with("enemy_"))
				stripped = stripped.substr(6);
			else if (stripped.starts_with("location_"))
				stripped = stripped.substr(9);
			else if (stripped.starts_with("item_"))
				stripped = stripped.substr(5);
			else if (stripped.starts_with("piece_"))
				stripped = stripped.substr(6);

			bool capitalize_next = true;
			for (char& c : stripped)
			{
				if (c == '_')
				{
					c = ' ';
					capitalize_next = true;
				}
				else if (capitalize_next && c >= 'a' && c <= 'z')
				{
					c = (char)std::toupper((unsigned char)c);
					capitalize_next = false;
				}
				else
				{
					capitalize_next = false;
				}
			}
			return stripped;
		}

		return raw_name;
	}

	void view::teleport_submenu()
	{
		canvas::add_tab<regular_submenu>("Teleport", SubmenuTeleport, [](regular_submenu* sub) {
			sub->add_option<reguler_option>("Teleport to Last Ping", "Teleports to the most recent map ping.", [] {
				g_fiber_pool->queue_job([] {
					auto ping = unity::get_last_ping();
					if (ping.has_value())
					{
						unity::teleport_to_world_point(ping.value());
						notification::success("Teleport", "Teleported to last ping.");
					}
					else
					{
						notification::warning("Teleport", "No active ping found on map.");
					}
				});
			});

			sub->add_option<reguler_option>("Teleport to Death Marker", "Teleports to your last death/tombstone location.", [] {
				g_fiber_pool->queue_job([] {
					auto death = unity::get_death_pin();
					if (death.has_value())
					{
						unity::teleport_to_world_point(death.value());
						notification::success("Teleport", "Teleported to death marker.");
					}
					else
					{
						notification::warning("Teleport", "No death marker found on map.");
					}
				});
			});

			sub->add_option<reguler_option>("Teleport to Bed / Spawn", "Teleports to your bed or spawn point.", [] {
				g_fiber_pool->queue_job([] {
					auto spawn = unity::get_spawn_pin();
					if (spawn.has_value())
					{
						unity::teleport_to_world_point(spawn.value());
						notification::success("Teleport", "Teleported to spawn point.");
					}
					else
					{
						notification::warning("Teleport", "No spawn point found on map.");
					}
				});
			});

			sub->add_option<sub_option>("Objectives (Bosses & Traders)", "Teleport to discovered bosses, traders, and special locations.", SubmenuObjectives);
			sub->add_option<sub_option>("Map Waypoints", "Teleport to custom pins/waypoints on the map.", SubmenuWaypoints);
			sub->add_option<sub_option>("Custom Teleport", nullptr, SubmenuCustomTeleport);

			sub->add_option<bool_option<bool>>("Ctrl + Middle Click Map Teleport", "Hold Ctrl and Middle Click anywhere on the map to teleport there.", &g_settings.self.map_click_teleport);
		});

		canvas::add_submenu<regular_submenu>("Objectives", SubmenuObjectives, [](regular_submenu* sub) {
			auto pins = unity::get_all_map_pins();
			int count = 0;

			for (const auto& pin : pins)
			{
				bool is_objective = (pin.type == 8 || pin.type == 9 || pin.type >= 14);
				if (!is_objective && !pin.name.empty())
				{
					std::string lower = pin.name;
					std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
					if (lower.find("haldor") != std::string::npos || lower.find("hildir") != std::string::npos || lower.find("witch") != std::string::npos || lower.find("bog") != std::string::npos || lower.find("trader") != std::string::npos || lower.find("boss") != std::string::npos || lower.find("eikthyr") != std::string::npos || lower.find("elder") != std::string::npos || lower.find("bonemass") != std::string::npos || lower.find("moder") != std::string::npos || lower.find("yagluth") != std::string::npos || lower.find("queen") != std::string::npos || lower.find("fader") != std::string::npos)
					{
						is_objective = true;
					}
				}

				if (is_objective)
				{
					count++;
					std::string default_label = "Objective";
					if (pin.type == 9)
						default_label = "Boss";
					else if (pin.type == 8)
						default_label = "Trader / Location";
					else if (pin.type >= 14)
						default_label = "Hildir Quest";

					std::string display_name = resolve_pin_display_name(pin.name, default_label, count);
					sub->add_option<reguler_option>(display_name.c_str(), nullptr, [pos = pin.pos, display_name] {
						g_fiber_pool->queue_job([pos] {
							unity::teleport_to_world_point(pos);
						});
						notification::success("Teleport", "Teleported to " + display_name);
					});
				}
			}

			if (count == 0)
			{
				sub->add_option<reguler_option>("No Objectives Discovered", nullptr, [] {
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Waypoints", SubmenuWaypoints, [](regular_submenu* sub) {
			auto pins = unity::get_all_map_pins();
			int count = 0;

			for (const auto& pin : pins)
			{
				if (pin.type <= 3 || pin.type == 6)
				{
					count++;
					std::string display_name = resolve_pin_display_name(pin.name, "Waypoint", count);
					sub->add_option<reguler_option>(display_name.c_str(), nullptr, [pos = pin.pos, display_name] {
						g_fiber_pool->queue_job([pos] {
							unity::teleport_to_world_point(pos);
						});
						notification::success("Teleport", "Teleported to " + display_name);
					});
				}
			}

			if (count == 0)
			{
				sub->add_option<reguler_option>("No Waypoints Found", nullptr, [] {
				});
			}
		});

		canvas::add_submenu<regular_submenu>("Custom Teleport", SubmenuCustomTeleport, [](regular_submenu* sub) {
			g_custom_teleport_service.fetch_saved_locations();
			sub->add_option<reguler_option>("Add Category", nullptr, [] {
				g_input_service.show("Input Category Name", [](std::string const& input) {
					teleport_location new_location;

					auto player = self::get_player();

					if (!player)
						return;

					auto coords = player.get_position();
					auto rotator = player.get_rotation();

					new_location.name = input;
					new_location.x = coords.x;
					new_location.y = coords.y;
					new_location.z = coords.z;
					new_location.rot_x = rotator.x;
					new_location.rot_y = rotator.y;
					new_location.rot_z = rotator.z;
					new_location.rot_w = rotator.w;

					g_custom_teleport_service.save_new_location(input, new_location);
				});
			});

			for (auto& l : g_custom_teleport_service.all_saved_locations | std::ranges::views::keys)
			{
				sub->add_option<sub_option>(l.c_str(), nullptr, joaat(l), [=] {
					category = l;
				});

				canvas::add_submenu<regular_submenu>(l.c_str(), joaat(l), [l](regular_submenu* sub) {
					std::vector<teleport_location> current_list{};
					current_list = g_custom_teleport_service.all_saved_locations.at(l);

					sub->add_option<reguler_option>("Add Teleport", nullptr, [l] {
						g_input_service.show("Input Location Name", [](std::string const& input) {
							teleport_location new_location;

							auto player = self::get_player();

							if (!player)
								return;

							auto coords = player.get_position();
							auto rotator = player.get_rotation();

							new_location.name = input;
							new_location.x = coords.x;
							new_location.y = coords.y;
							new_location.z = coords.z;
							new_location.rot_x = rotator.x;
							new_location.rot_y = rotator.y;
							new_location.rot_z = rotator.z;
							new_location.rot_w = rotator.w;

							g_custom_teleport_service.save_new_location(category, new_location);
						});
					});

					for (const auto& location : current_list)
					{
						sub->add_option<reguler_option>(location.name.c_str(), nullptr, [=] {
							g_fiber_pool->queue_job([=] {
								auto player = self::get_player();

								if (!player)
									return;

								player.teleport_to(Vector3(location.x, location.y + 3.f, location.z), Vector4(location.rot_x, location.rot_y, location.rot_z, location.rot_w), true);
							});
						});
					}
				});
			}
		});
	}
}
