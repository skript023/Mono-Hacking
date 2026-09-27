#include "commands/looped_command.hpp"
#include "commands/int_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	int_command _inventory_width("inventory_width", "Inventory Width", "Width of the inventory.", 8, 1000, 8);
	int_command _inventory_height("inventory_height", "Inventory Height", "Height of the inventory.", 4, 1000, 4);

	static void update_gui_size(int rows)
	{
		MonoClass* gui_class = mono::get_class("InventoryGui", "assembly_valheim");
		if (!gui_class)
			return;

		MonoClassField* inst_field = mono::get_field(gui_class, "m_instance");
		if (!inst_field)
			return;

		void* static_data = mono::get_static_field_data(gui_class);
		if (!static_data)
			return;

		uint32_t offset = mono::get_field_offset(inst_field);
		void* ptr_addr = (void*)((uintptr_t)static_data + offset);
		if (!ptr_addr)
			return;

		MonoObject* gui_inst = *(MonoObject**)ptr_addr;
		if (!gui_inst)
			return;

		static auto set_size_method = mono::get_method("InventoryGui", "SetInventorySize", 1, "assembly_valheim");
		if (!set_size_method)
			return;

		mono::invoke(set_size_method, gui_inst, rows);
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

		virtual void on_tick() override
		{
			auto player = self::get_player();
			if (!player.get_object())
				return;

			auto inventory = player.get_inventory();
			if (!inventory.get_object())
				return;

			int height = _inventory_height.get_state();
			int width = _inventory_width.get_state();

			int max_y = get_max_occupied_y(inventory.get_object());
			int target_height = std::max(height, max_y + 1);

			inventory.set_height(target_height);
			inventory.set_width(width);

			update_gui_size(target_height);
			sync_player_invrows(player.get_object(), target_height);
		}

		virtual void on_disable() override
		{
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
					update_gui_size(safe_height);
					sync_player_invrows(player.get_object(), safe_height);
				}
			}
		}
	};

	static inventory_size _inventory_size("inventory_size", "Inventory Size", "Size of the inventory");
}
