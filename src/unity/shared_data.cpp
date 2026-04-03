#include "shared_data.hpp"

namespace big
{
	shared_data::shared_data(MonoObject* o): m_shared(o)
	{}
	shared_data::~shared_data() noexcept
	{
        m_shared = nullptr;
	}
    float shared_data::get_max_durability()
	{
		return mono::get_field_value<float>(m_shared, "ItemDrop/ItemData/SharedData", "m_maxDurability");
	}
    float shared_data::get_durability_per_level()
	{
		return mono::get_field_value<float>(m_shared, "ItemDrop/ItemData/SharedData", "m_durabilityPerLevel");
	}
}