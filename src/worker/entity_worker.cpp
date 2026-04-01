#include "entity_worker.hpp"
#include "script.hpp"

#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "unity/item_drop.hpp"
namespace big
{
    void entity_worker::run()
	{
		while (g_running)
		{
			TRY_CLAUSE
			{
				auto& back = g_esp_data.back(); back.clear();

				auto player = self::get_player();

				auto characters = character::get_all_characters();
				auto drops = item_drop::get_drops();

				auto local_player_pos = player.get_position();

				for (auto& drop : drops)
				{
					if (!drop || (uintptr_t)drop.get_object() < 0x10000)
						continue;

					auto pos = drop.get_position();
					
					if (!pos.has_value())
						continue;

					auto name = drop.get_hover_name();
					auto health = 0.f;
					auto max_health = 0.f;
					auto distance = local_player_pos.distance_in_meters(*pos);
					
					Vector3 screen;
					if (!unity::world_to_screen(*pos, screen))
						continue;

					char buffer[256];
					snprintf(buffer, sizeof(buffer), "%s [%.2f]m", name.c_str(), distance);

					esp_data data{};
					data.location = *pos;
					data.screen = screen;
					data.name = buffer;
					data.health = health;
					data.max_health = max_health;
					data.distance = distance;
					data.type = EEntityType::ItemDrop;
					back.push_back(data);
				}

				for (auto& character : characters)
				{
					if (!character || (uintptr_t)character.get_object() < 0x10000 || character == player)
						continue;
#ifdef _DEBUG
					MonoClass* elem_class = mono::object_get_class(player);

					LOG(INFO) << "Element class: "
						<< mono::class_get_namespace(elem_class)
						<< "::"
						<< mono::class_get_name(elem_class) << " at address " << player << " local player is " << player;
#endif

					Vector3 pos = character.get_position();
					
					if (pos.is_zero())
						continue;

					auto top = character.get_top_point();
					auto name = character.get_hover_name();
					auto health = character.get_health();
					auto max_health = character.get_max_health();
					auto distance = local_player_pos.distance_in_meters(pos);
					
					Vector3 screen;
					if (!unity::world_to_screen(pos, screen))
						continue;

					char buffer[256];
					snprintf(buffer, sizeof(buffer), "%s [%.2f]m", name.c_str(), distance);

					esp_data data{};
					data.location = pos;
					data.screen = screen;
					data.name = buffer;
					data.health = health;
					data.max_health = max_health;
					data.distance = distance;
					data.top = top;
					data.bottom = pos;
					data.type = EEntityType::Character;
					back.push_back(data);
				}

				g_esp_data.publish();
			} 
			EXCEPT_CLAUSE
			script::get_current()->yield(100ms);
		}
	}
} // namespace big
