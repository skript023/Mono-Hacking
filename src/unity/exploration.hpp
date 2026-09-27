#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include "minimap.hpp"
#include "zone_system.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace big
{
	class exploration
	{
	public:
		struct location_entry
		{
			std::vector<std::string> candidate_names;
			std::string display_name;
			std::string pin_name;
			int pin_type;
			std::string biome;
		};

		struct options
		{
			bool discover_all{false};
		};

		static void set_options(const options& opt)
		{
			return instance().set_options_impl(opt);
		}
		static options get_options()
		{
			return instance().get_options_impl();
		}

		static void explore_all_map()
		{
			return instance().explore_all_map_impl();
		}
		static void reset_map()
		{
			return instance().reset_map_impl();
		}

		static bool discover_location(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all)
		{
			return instance().discover_location_impl(name, pin_name, pin_type, discover_all);
		}
		static bool discover_location(const location_entry& entry, bool discover_all)
		{
			return instance().discover_location_impl(entry, discover_all);
		}
		static void discover_all_bosses(bool discover_all)
		{
			return instance().discover_all_bosses_impl(discover_all);
		}
		static void discover_all_traders(bool discover_all)
		{
			return instance().discover_all_traders_impl(discover_all);
		}
		static void discover_everything(bool discover_all)
		{
			return instance().discover_everything_impl(discover_all);
		}

		static const std::vector<location_entry>& get_boss_entries()
		{
			return instance().get_boss_entries_impl();
		}
		static const std::vector<location_entry>& get_trader_entries()
		{
			return instance().get_trader_entries_impl();
		}

	private:
		exploration() = default;
		exploration(const exploration&) = delete;
		exploration& operator=(const exploration&) = delete;
		static exploration& instance()
		{
			static exploration value;
			return value;
		}

		void set_options_impl(const options& opt);
		options get_options_impl();
		void explore_all_map_impl();
		void reset_map_impl();
		bool discover_location_impl(std::string_view name, std::string_view pin_name, int pin_type, bool discover_all);
		bool discover_location_impl(const location_entry& entry, bool discover_all);
		void discover_all_bosses_impl(bool discover_all);
		void discover_all_traders_impl(bool discover_all);
		void discover_everything_impl(bool discover_all);
		const std::vector<location_entry>& get_boss_entries_impl();
		const std::vector<location_entry>& get_trader_entries_impl();

		options g_options;
	};
}
