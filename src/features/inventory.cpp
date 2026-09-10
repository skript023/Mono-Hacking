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

			inventory.set_height(height);
			inventory.set_width(width);

			update_gui_size(height);
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();
			if (player.get_object())
			{
				auto inventory = player.get_inventory();
				if (inventory.get_object())
				{
					inventory.set_height(4);
					inventory.set_width(8);
				}
			}

			update_gui_size(4);
		}
	};

	static inventory_size _inventory_size("inventory_size", "Inventory Size", "Size of the inventory");
}
