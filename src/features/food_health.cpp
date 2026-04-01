#include "commands/looped_command.hpp"
#include "commands/float_command.hpp"
#include "mono/mono.hpp"

#include "unity/self.hpp"
#include "utility/unity.hpp"

namespace big::features
{
	float_command _food_hp("food_hp", "Food Health Amount", "Amount of food health.", 25.f, 10000000.f, 25.f);

	static void set_all_food_health(MonoObject* foodsList, float newHealth)
	{
		if (!foodsList)
			return;

		auto foods = unity::list_to_vector(foodsList);
		if (foods.empty())
			return;

		// Ambil field m_health dari class Food yang BENAR
		MonoClass* foodClass = mono::get_class("Player/Food", "assembly_valheim");
		if (!foodClass)
		{
			LOG(WARNING) << "Failed to get Food class.";
			return;
		}

		auto healthField = mono::get_field(foodClass, "m_time");
		if (!healthField)
		{
			LOG(WARNING) << "Failed to get m_health field from food class.";
			return;
		}

		for (MonoObject* food : foods)
		{
			if (!food)
			{
				LOG(WARNING) << "Food object is empty.";
				continue;
			}
#ifdef _DEBUG
			LOG(VERBOSE) << food << " setting food health to " << newHealth;
#endif
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
			// MonoObject* player = unity::get_local_player(); // sesuaikan fungsi kamu
			// if (!player)
			// 	return;

			// // Panggil GetFoods()
			// MonoMethod* method = mono::get_method("Player", "GetFoods", 0, "assembly_valheim");

			// MonoObject* foodsList = mono::invoke_method(method, player);
			// if (!foodsList)
			// {
			// 	LOG(WARNING) << "Failed to get food list object.";
			// 	return;
			// }

			// // Set health semua Food
			// set_all_food_health(foodsList, _food_hp.get_state());
			auto player = self::get_player();
			
			auto foods = player.get_foods();

			for (auto& food : foods)
			{
				food.set_time(_food_hp.get_state());
			}
		}

		virtual void on_disable() override
		{

		}
	};

	static food_health _food_health("max_food_health", "Food Timer", "Amount of food time.");
}