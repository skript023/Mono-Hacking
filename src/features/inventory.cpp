#include "commands/looped_command.hpp"
#include "commands/int_command.hpp"
#include "mono/mono.hpp"

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
			auto method = mono::get_method("Humanoid", "GetInventory", 0, "assembly_valheim");
            auto player = unity::get_local_player();

            if (!method || !player) 
            {
                LOG(WARNING) << "Failed to find method Humanoid::GetInventory or local player";
                return;
            }

            auto ret = mono::invoke_method(method, player);

            if (ret == 0)
                return;

            auto inventory = mono::get_class("Inventory", "assembly_valheim");
            auto m_height = mono::get_field(inventory, "m_height");
            auto m_width = mono::get_field(inventory, "m_width");
            mono::set_field_value(ret, m_height, &_inventory_height.get_state());
            mono::set_field_value(ret, m_width, &_inventory_width.get_state());
		}

		virtual void on_disable() override
		{
			auto klass = mono::get_class("Player", "assembly_valheim");
			auto m_maxCarryWeight = mono::get_field(klass, "m_maxCarryWeight");

			if (!klass || !m_maxCarryWeight)
			{
				LOG(WARNING) << "Failed to find method Player::GetPlayerName";

				return;
			}

			float default_stamina_regen = 0.f;

			mono::set_field_value(unity::get_local_player(), m_maxCarryWeight, &default_stamina_regen);
		}
	};

	static inventory_size _inventory_size("inventory_size", "Inventory Size", "Size of the inventory");
}