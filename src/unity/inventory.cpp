#include "inventory.hpp"
#include "utility/unity.hpp"

namespace big
{
	inventory::inventory(MonoObject* o) :
	    m_inventory(o)
	{
	}
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
	void inventory::sanitize_all()
	{
		if (!m_inventory)
			return;

		static auto get_all_method = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
		if (!get_all_method)
			return;

		auto all_items = mono::invoke_method(get_all_method, m_inventory, nullptr);
		if (!all_items)
			return;

		auto items_vec = unity::list_to_vector(all_items);
		for (auto* itm_obj : items_vec)
		{
			if (!itm_obj)
				continue;
			bool cheated = mono::get_field_value<"ItemDrop/ItemData", "m_cheated", bool>(itm_obj);
			if (cheated)
			{
				mono::set_field_value<"ItemDrop/ItemData", "m_cheated">(itm_obj, false);
			}
		}
	}
}
