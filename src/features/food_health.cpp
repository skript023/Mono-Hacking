#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "utility/unity.hpp"

namespace big::features
{
	float_command _food_hp("food_hp", "Food Health Amount", "Amount of food health.", 25.f, 1000.f, 25.f);

	static void set_all_food_health(MonoObject* foodsList, float newHealth)
	{
		if (!foodsList)
			return;

		auto foods = unity::list_to_vector(foodsList);
		if (foods.empty())
			return;

		// Ambil field m_health dari Food class
		MonoClass* foodClass = mono::get_class("Food", "assembly_valheim");
		auto healthField = mono::get_field(foodClass, "m_health");
		if (!healthField)
			return;

		for (MonoObject* food : foods)
		{
			mono::set_field_value(food, healthField, &newHealth);
		}
	}

	class food_health : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_call() override
		{
			if (m_state)
				on_enable();
			else
				on_disable();
		}

		virtual void on_tick() override
		{
			MonoObject* player = unity::get_local_player(); // sesuaikan fungsi kamu
			if (!player)
				return;

			// Panggil GetFoods()
			MonoMethod* method = mono::get_method("Player", "GetFoods", 0, "assembly_valheim");
			MonoObject* exc = nullptr;
			MonoObject* foodsList = mono::invoke_method(method, player);
			if (!foodsList || exc)
				return;

			// Set health semua Food
			set_all_food_health(foodsList, _food_hp.get_state());
		}

		virtual void on_disable() override
		{

		}
	};

	static food_health _food_health("max_food_health", "Food Health Amount", "Amount of food health.");
}