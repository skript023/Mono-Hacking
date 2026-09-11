#include "food.hpp"

namespace big
{
	food::food(MonoObject* food): m_food(food)
	{}

	food::~food() noexcept
	{
		m_food = nullptr;
	}

	std::string food::get_name()
	{
        return mono::get_field_value<"Player/Food", "m_name", std::string>(m_food);
	}

	std::string food::get_shared_name()
	{
		if (!m_food)
			return "";

		static auto item_field = mono::get_field("Player/Food", "m_item", "assembly_valheim");
		static uint32_t item_offset = item_field ? mono::get_field_offset(item_field) : 0;
		MonoObject* item_obj = item_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)m_food + item_offset) : nullptr;
		if (!item_obj)
			return "";

		static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
		static uint32_t shared_offset = shared_field ? mono::get_field_offset(shared_field) : 0;
		MonoObject* shared_obj = shared_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item_obj + shared_offset) : nullptr;
		if (!shared_obj)
			return "";

		static auto name_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_name", "assembly_valheim");
		static uint32_t name_offset = name_field ? mono::get_field_offset(name_field) : 0;
		MonoString* name_mono = (shared_obj && name_offset) ? *reinterpret_cast<MonoString**>((uintptr_t)shared_obj + name_offset) : nullptr;
		return name_mono ? mono::from_mono_string(name_mono) : "";
	}

	float food::get_time()
	{
		return mono::get_field_value<"Player/Food", "m_time", float>(m_food);
	}
	float food::get_health()
	{
		return mono::get_field_value<"Player/Food", "m_health", float>(m_food);
	}
	float food::get_stamina()
	{
		return mono::get_field_value<"Player/Food", "m_stamina", float>(m_food);
	}
	float food::get_eitr()
	{
		return mono::get_field_value<"Player/Food", "m_eitr", float>(m_food);
	}
	void food::set_time(float time)
	{
		if (!mono::set_field_value<"Player/Food", "m_time">(m_food, time))
		{
			LOG(FATAL) << "Failed set field m_time";
		}
	}
	void food::set_health(float h)
	{
		if (!mono::set_field_value<"Player/Food", "m_health">(m_food, h))
		{
			LOG(FATAL) << "Failed set field m_health";
		}
	}
	void food::set_stamina(float s)
	{
		if (!mono::set_field_value<"Player/Food", "m_stamina">(m_food, s))
		{
			LOG(FATAL) << "Failed set field m_stamina";
		}
	}
	void food::set_eitr(float e)
	{
		if (!mono::set_field_value<"Player/Food", "m_eitr">(m_food, e))
		{
			LOG(FATAL) << "Failed set field m_eitr";
		}
	}
	bool food::can_eat_again()
	{
		static MonoMethod* method = mono::get_method("Player/Food", "CanEatAgain", 0, "assembly_valheim");

		if (method && m_food)
		{
			MonoObject* result = mono::invoke_method(method, m_food);
			if (result)
			{
				auto value = mono::object_unbox(result);
				if (value)
					return *static_cast<bool*>(value);
			}
		}

		// Fallback: m_time < m_item.m_shared.m_foodBurnTime / 2f
		float t = get_time();
		static auto item_field = mono::get_field("Player/Food", "m_item", "assembly_valheim");
		static uint32_t item_offset = item_field ? mono::get_field_offset(item_field) : 0;
		MonoObject* item_obj = item_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)m_food + item_offset) : nullptr;
		if (item_obj)
		{
			static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
			static uint32_t shared_offset = shared_field ? mono::get_field_offset(shared_field) : 0;
			MonoObject* shared_obj = shared_offset ? *reinterpret_cast<MonoObject**>((uintptr_t)item_obj + shared_offset) : nullptr;
			if (shared_obj)
			{
				static auto burn_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_foodBurnTime", "assembly_valheim");
				static uint32_t burn_offset = burn_field ? mono::get_field_offset(burn_field) : 0;
				float burn = burn_offset ? *reinterpret_cast<float*>((uintptr_t)shared_obj + burn_offset) : 0.f;
				if (burn > 0.f)
					return t < (burn / 2.f);
			}
		}

		return false;
	}
}
