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
        return mono::get_field_value<std::string>(m_food, "Player/Food", "m_name");
	}
	float food::get_time()
	{
		return mono::get_field_value<float>(m_food, "Player/Food", "m_time");
	}
	float food::get_health()
	{
		return mono::get_field_value<float>(m_food, "Player/Food", "m_health");
	}
	float food::get_stamina()
	{
		return mono::get_field_value<float>(m_food, "Player/Food", "m_stamina");
	}
	float food::get_eitr()
	{
		return mono::get_field_value<float>(m_food, "Player/Food", "m_eitr");
	}
	void food::set_time(float time)
	{
		if (!mono::set_field_value(m_food, "Player/Food", "m_time", time))
		{
			LOG(FATAL) << "Failed set field m_time";
		}
	}
	void food::set_health(float h)
	{
		if (!mono::set_field_value(m_food, "Player/Food", "m_health", h))
		{
			LOG(FATAL) << "Failed set field m_health";
		}
	}
	void food::set_stamina(float s)
	{
		if (!mono::set_field_value(m_food, "Player/Food", "m_stamina", s))
		{
			LOG(FATAL) << "Failed set field m_stamina";
		}
	}
	void food::set_eitr(float e)
	{
		if (!mono::set_field_value(m_food, "Player/Food", "m_eitr", e))
		{
			LOG(FATAL) << "Failed set field m_eitr";
		}
	}
	bool food::can_eat_again()
	{
		static MonoMethod* method = mono::get_method("Player/Food", "CanEatAgain", 0, "assembly_valheim");

		if (!method)
			return false;

		MonoObject* result = mono::invoke_method(method);

        return *reinterpret_cast<bool*>(mono::object_unbox(result));
	}
}