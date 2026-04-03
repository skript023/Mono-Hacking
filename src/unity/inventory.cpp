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
        if (!mono::set_field_value(m_inventory, "Inventory", "m_height", height))
        {
            LOG(FATAL) << "Failed set m_height";
        }
	}
	void inventory::set_width(int width)
	{
        if (!mono::set_field_value(m_inventory, "Inventory", "m_width", width))
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