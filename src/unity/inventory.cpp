#include "inventory.hpp"

namespace big
{
    inventory::inventory(MonoObject* o): m_inventory(o)
	{}
	inventory::~inventory() noexcept
	{
        m_inventory = nullptr;
	}
	void inventory::set_height(int height)
	{
        if (!mono::set_field_value<"Inventory", "m_height">(m_inventory, height))
        {
            LOG(FATAL) << "Failed set m_height";
        }
	}
	void inventory::set_width(int width)
	{
        if (!mono::set_field_value<"Inventory", "m_width">(m_inventory, width))
        {
            LOG(FATAL) << "Failed set m_width";
        }
	}
	int inventory::get_height()
	{
		return mono::get_field_value<"Inventory", "m_height", int>(m_inventory);
	}
	int inventory::get_width()
	{
		return mono::get_field_value<"Inventory", "m_width", int>(m_inventory);
	}
}