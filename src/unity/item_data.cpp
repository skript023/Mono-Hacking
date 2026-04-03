#include "item_data.hpp"

namespace big
{
	item_data::item_data(MonoObject* o): obj(o)
	{
	}
	item_data::~item_data() noexcept
	{
		obj = nullptr;
	}
	MonoObject* item_data::get_object()
	{
		return obj;
	}
	void item_data::set_durability(float durability)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_durability", durability))
		{
			LOG(FATAL) << "Failed set durability";
		}
	}
	void item_data::set_quality(int quality)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_quality", quality))
		{
			LOG(FATAL) << "Failed set quality";
		}
	}
	void item_data::set_stack(int stack)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_stack", stack))
		{
			LOG(FATAL) << "Failed set stack";
		}
	}
	void item_data::set_variant(int variant)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_variant", variant))
		{
			LOG(FATAL) << "Failed set variant";
		}
	}
	void item_data::set_grid_pos(iVector2 pos)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_gridPos", pos))
		{
			LOG(FATAL) << "Failed set grid pos";
		}
	}
}