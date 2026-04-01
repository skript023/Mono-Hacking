#include "food.hpp"

namespace big
{
	food::food(MonoObject* food): m_food(food)
	{
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
	bool food::can_eat_again()
	{
		static MonoMethod* method = mono::get_method("Character", "GetAllCharacters", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke_method(method, nullptr);

        return *reinterpret_cast<bool*>(mono::object_unbox(result));
	}
}