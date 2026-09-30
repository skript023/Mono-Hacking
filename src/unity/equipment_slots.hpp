#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"
#include "imgui.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace big
{
	enum class slot_kind
	{
		none = 0,
		helmet,
		chest,
		legs,
		cape,
		trinket,
		utility,
		food,
		ammo,
		key,
		quick
	};

	struct slot_coord
	{
		int x;
		int y;
	};

	class equipment_manager
	{
	public:
		static constexpr int grid_width = 8;
		static constexpr int grid_height = 5;
		static constexpr int page_count = 6;
		static constexpr int equipment_page_idx = 5;
		static constexpr int max_extra_utilities = 3; // 1 vanilla + 3 extras = 4 total

		static equipment_manager& get()
		{
			static equipment_manager instance;
			return instance;
		}

		// Slot layout and validation
		static slot_kind get_slot_kind(int x, int y);
		static const char* get_slot_kind_name(slot_kind kind);
		static int get_slot_number(slot_kind kind, int x, int y);
		static iVector2 get_slot_pos(slot_kind kind, int number);
		static bool fits_slot(slot_kind kind, MonoObject* item_data_obj);
		static bool fits_slot(iVector2 pos, MonoObject* item_data_obj);
		static iVector2 find_home_slot(MonoObject* equipment_inv, MonoObject* item_data_obj);

		// Lifecycle & Persistence
		void init_or_sync();
		void save_to_player();
		void tick(MonoObject* player);

		// Slot actions
		MonoObject* get_equipment_inventory();
		MonoObject* get_item_at(int x, int y);
		bool move_to_slot(MonoObject* from_inv, MonoObject* item, int to_x, int to_y);
		bool move_from_slot_to_inventory(int from_x, int from_y);
		void use_slot(slot_kind kind, int number);
		void use_hotbar_index(int index, bool alt_held, bool shift_held);
		void auto_fill_from_inventory();
		void deposit_all_to_inventory();
		void unequip_all();

		// Hooks callbacks
		bool handle_inventory_equip(MonoObject* humanoid, MonoObject* item);
		bool on_equip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects);
		bool on_unequip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects);
		void on_update_equipment_status_effects(MonoObject* humanoid);
		void on_unequip_all(MonoObject* humanoid);
		void on_player_save(MonoObject* player, MonoObject* pkg);

		bool is_in_equip() const
		{
			return m_in_equip;
		}

		// UI Overlay
		static void draw_ui();

	private:
		equipment_manager() = default;
		~equipment_manager();

		MonoObject* create_page(int index);
		void release_handles();
		void ensure_pages();
		void sync_equipment_with_humanoid(MonoObject* humanoid);
		void send_to_slot(MonoObject* item);

		MonoObject* m_pages[page_count]{};
		uintptr_t m_page_handles[page_count]{};
		std::vector<MonoObject*> m_extra_utilities;

		MonoObject* m_cached_player{nullptr};
		bool m_initialized{false};
		bool m_in_equip{false};
		bool m_syncing{false};
		bool m_saving{false};

		// Ammo hotkey debounce
		bool m_r_key_down{false};
		bool m_t_key_down{false};
		bool m_z_key_down{false};

		// UI state
		int m_selected_slot_x{-1};
		int m_selected_slot_y{-1};
		bool m_show_picker{false};
	};
}
