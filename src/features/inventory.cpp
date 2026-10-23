#include "commands/looped_command.hpp"
#include "commands/int_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	int_command _inventory_width("inventory_width", "Inventory Width", "Width of the inventory.", 8, 1000, 8);
	int_command _inventory_height("inventory_height", "Inventory Height", "Height of the inventory.", 4, 1000, 4);

	static MonoObject* inventory_gui()
	{
		static auto slot = static_cast<MonoObject**>(mono::get_static_field_value("InventoryGui", "m_instance", "assembly_valheim"));
		return slot ? *slot : nullptr;
	}

	static void update_gui_size(MonoObject* gui, int rows)
	{
		static auto set_size_method = mono::get_method("InventoryGui", "SetInventorySize", 1, "assembly_valheim");
		if (gui && set_size_method)
		{
			mono::invoke(set_size_method, gui, rows);
		}
	}

	static int get_max_occupied_y(MonoObject* inv_obj)
	{
		if (!inv_obj)
			return 3;

		int max_y = 3;
		static auto get_all_method = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
		if (get_all_method)
		{
			auto all_items = mono::invoke_method(get_all_method, inv_obj, nullptr);
			if (all_items)
			{
				auto vec = unity::list_to_vector(all_items);
				for (auto* itm : vec)
				{
					if (!itm)
						continue;
					auto pos = mono::get_field_value<"ItemDrop/ItemData", "m_gridPos", iVector2>(itm);
					if (pos.y > max_y)
						max_y = pos.y;
				}
			}
		}
		return max_y;
	}

	static void sync_player_invrows(MonoObject* player_obj, int rows)
	{
		if (!player_obj)
			return;

		static auto add_unique_method = mono::get_method("Player", "AddUniqueKeyValue", 2, "assembly_valheim");
		if (add_unique_method)
		{
			auto ms_key = mono::to_mono_string("invrows");
			auto ms_val = mono::to_mono_string(std::to_string(std::clamp(rows, 4, 9)));
			void* uargs[2] = {ms_key, ms_val};
			mono::invoke_method(add_unique_method, player_obj, uargs);
		}
	}

	class inventory_size : public looped_command
	{
		using looped_command::looped_command;
		MonoObject* m_inventory = nullptr;
		MonoObject* m_gui       = nullptr;
		int m_width             = 0;
		int m_height            = 0;
		int m_applied_height    = 0;

		virtual void on_tick() override
		{
			auto player = self::get_player();
			if (!player.get_object())
				return;

			auto inventory = player.get_inventory();
			if (!inventory.get_object())
				return;

			const int height = _inventory_height.get_state();
			const int width  = _inventory_width.get_state();
			if (inventory.get_object() != m_inventory || width != m_width || height != m_height || inventory.get_width() != width
			    || inventory.get_height() != m_applied_height)
			{
				const int target_height = std::max(height, get_max_occupied_y(inventory.get_object()) + 1);
				inventory.set_height(target_height);
				inventory.set_width(width);
				sync_player_invrows(player.get_object(), target_height);

				m_inventory      = inventory.get_object();
				m_width          = width;
				m_height         = height;
				m_applied_height = target_height;
				m_gui            = nullptr;
			}

			auto gui = inventory_gui();
			if (gui && gui != m_gui)
			{
				update_gui_size(gui, m_applied_height);
				m_gui = gui;
			}
		}

		virtual void on_disable() override
		{
			m_inventory = nullptr;
			m_gui       = nullptr;
			auto player = self::get_player();
			if (player.get_object())
			{
				auto inventory = player.get_inventory();
				if (inventory.get_object())
				{
					int max_y = get_max_occupied_y(inventory.get_object());
					int safe_height = std::max(4, max_y + 1);

					inventory.set_height(safe_height);
					inventory.set_width(8);
					update_gui_size(inventory_gui(), safe_height);
					sync_player_invrows(player.get_object(), safe_height);
				}
			}
		}
	};

	static inventory_size _inventory_size("inventory_size", "Inventory Size", "Size of the inventory");
}
