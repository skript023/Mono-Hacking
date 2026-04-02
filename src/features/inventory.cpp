#include "commands/looped_command.hpp"
#include "commands/int_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	int_command _inventory_width("inventory_width", "Inventory Width", "Width of the inventory.", 8, 1000, 8);
	int_command _inventory_height("inventory_height", "Inventory Height", "Height of the inventory.", 4, 1000, 4);
    
	class inventory_size : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto player = self::get_player();

            auto inventory = player.get_inventory();
            inventory.set_height(_inventory_height.get_state());
            inventory.set_width(_inventory_width.get_state());
		}

		virtual void on_disable() override
		{
			auto player = self::get_player();

            auto inventory = player.get_inventory();
            inventory.set_height(4);
            inventory.set_width(8);
		}
	};

	static inventory_size _inventory_size("inventory_size", "Inventory Size", "Size of the inventory");
}