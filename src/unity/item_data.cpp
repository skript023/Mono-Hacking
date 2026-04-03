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
	shared_data item_data::get_shared_data()
	{
		return mono::get_field_value<MonoObject*>(obj, "ItemDrop/ItemData", "m_shared");
	}
	void item_data::set_durability(float durability)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_durability", durability))
		{
			LOG(FATAL) << "Failed set durability";
		}
	}
	float item_data::get_durability()
	{
		return mono::get_field_value<float>(obj, "ItemDrop/ItemData", "m_durability");
	}
	float item_data::get_max_durability()
	{
		auto shared = get_shared_data();

		auto durability = shared.get_max_durability();
		auto durability_per_level = shared.get_durability_per_level();
		auto quality = mono::get_field_value<int>(obj, "ItemDrop/ItemData", "m_quality");

		return durability + (float)std::max(0, quality - 1) * durability_per_level; 
	}
	void item_data::set_quality(int quality)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_quality", quality))
		{
			LOG(FATAL) << "Failed set quality";
		}
	}
	int item_data::get_quality()
	{
		return mono::get_field_value<int>(obj, "ItemDrop/ItemData", "m_quality");
	}
	void item_data::set_stack(int stack)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_stack", stack))
		{
			LOG(FATAL) << "Failed set stack";
		}
	}
	int item_data::get_stack()
	{
		return mono::get_field_value<int>(obj, "ItemDrop/ItemData", "m_stack");
	}
	void item_data::set_variant(int variant)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_variant", variant))
		{
			LOG(FATAL) << "Failed set variant";
		}
	}
	int item_data::get_variant()
	{
		return mono::get_field_value<int>(obj, "ItemDrop/ItemData", "m_variant");
	}
	void item_data::set_grid_pos(iVector2 pos)
	{
		if (!mono::set_field_value(obj, "ItemDrop/ItemData", "m_gridPos", pos))
		{
			LOG(FATAL) << "Failed set grid pos";
		}
	}
	iVector2 item_data::get_grid_pos()
	{
		return mono::get_field_value<iVector2>(obj, "ItemDrop/ItemData", "m_gridPos");
	}
}