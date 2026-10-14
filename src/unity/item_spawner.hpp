#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "mono/mono.hpp"
#include <string>
#include <vector>

namespace big
{
	class item_spawner
	{
	public:
		struct item_entry
		{
			std::string prefab_name;
			std::string display_name;
			std::string category;
			int max_stack{1};
			int max_quality{1};
			int variant_count{1};
		};

		static void initialize()
		{
			return instance().initialize_impl();
		}
		static void refresh()
		{
			return instance().refresh_impl();
		}
		static bool is_refreshing()
		{
			return instance().is_refreshing_impl();
		}
		static std::vector<item_entry> get_items()
		{
			return instance().get_items_impl();
		}
		static std::vector<std::string> get_categories()
		{
			return instance().get_categories_impl();
		}
		static bool spawn_to_inventory(const std::string& prefab_name, int amount, int quality, int variant = 0, float durability = 1.f, bool auto_equip = false)
		{
			return instance().spawn_to_inventory_impl(prefab_name, amount, quality, variant, durability, auto_equip);
		}
		static bool spawn_in_world(const std::string& prefab_name, int amount, int level = 1, int variant = 0, float durability = 1.f, int placement = 0)
		{
			return instance().spawn_in_world_impl(prefab_name, amount, level, variant, durability, placement);
		}
		static void draw_menu_ui()
		{
			return instance().draw_menu_ui_impl();
		}
		static void draw_standalone_window()
		{
			return instance().draw_standalone_window_impl();
		}
		static void toggle_standalone_window()
		{
			return instance().toggle_standalone_window_impl();
		}
		static bool is_standalone_window_open()
		{
			return instance().is_standalone_window_open_impl();
		}
		static void focus_item(const std::string& prefab_name)
		{
			return instance().focus_item_impl(prefab_name);
		}

	private:
		item_spawner()                               = default;
		item_spawner(const item_spawner&)            = delete;
		item_spawner& operator=(const item_spawner&) = delete;
		static item_spawner& instance()
		{
			static item_spawner value;
			return value;
		}

		void initialize_impl();
		void persist_history_impl();
		void refresh_impl();
		bool is_refreshing_impl();
		std::vector<item_entry> get_items_impl();
		std::vector<std::string> get_categories_impl();
		bool spawn_to_inventory_impl(const std::string& prefab_name, int amount, int quality, int variant, float durability, bool auto_equip);
		bool spawn_in_world_impl(const std::string& prefab_name, int amount, int level, int variant, float durability, int placement);
		void draw_menu_ui_impl();
		void draw_standalone_window_impl();
		void toggle_standalone_window_impl();
		bool is_standalone_window_open_impl();
		void focus_item_impl(const std::string& prefab_name);

		std::vector<item_entry> g_items;
		std::unordered_set<std::string> s_favorites;
		std::vector<std::string> s_recent_items;
		std::string s_selected_prefab;
		std::string s_last_spawned_prefab;
		int s_last_spawn_amount       = 1;
		int s_last_spawn_quality      = 1;
		int s_last_spawn_variant      = 0;
		float s_last_spawn_durability = 1.f;
		int s_last_spawn_placement    = 0;
		bool s_last_spawn_auto_equip  = false;
		int s_spawn_variant           = 0;
		float s_spawn_durability      = 1.f;
		bool s_auto_equip             = false;
		int s_spawn_placement         = 0;
		bool s_last_spawn_to_world    = false;
		std::mutex g_items_mutex;
		bool g_initialized = false;
		std::atomic<bool> s_is_refreshing{false};
		std::atomic<bool> s_auto_scan_started{false};
		std::once_flag s_history_load_once;
		char s_search_buffer[64]  = "";
		int s_selected_category   = 0;
		int s_spawn_amount        = 1;
		int s_spawn_quality       = 1;
		int s_selected_item_index = 0;
		bool s_favorites_only     = false;
		bool s_recent_only        = false;
		bool s_standalone_open    = false;

		void scan_worker_impl();
		void spawn_to_inventory_worker_impl(const std::string& prefab_name, int amount, int quality, int variant, float durability, bool auto_equip);
		void spawn_in_world_worker_impl(const std::string& prefab_name, int amount, int level, int variant, float durability, int placement);
	};
}
