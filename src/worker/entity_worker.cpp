#include "entity_worker.hpp"
#include "script.hpp"

#include "utility/joaat.hpp"
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

				auto self = self::get_player();

				static auto last_list_update = std::chrono::steady_clock::now();
				static mono_array_view<character> characters;
				
				if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_list_update).count() > 1000) 
				{
					characters = character::get_all_characters();
					last_list_update = std::chrono::steady_clock::now();
				}

				auto local_player_pos = self.get_position();

				for (auto character : characters)
				{
					if (!character || (uintptr_t)character.get_object() < 0x10000)
						continue;

					auto classname = mono::get_name(character.get_object());
#ifdef _DEBUG
					MonoClass* elem_class = mono::object_get_class(player);

					LOG(INFO) << "Element class: "
						<< mono::class_get_namespace(elem_class)
						<< "::"
						<< mono::class_get_name(elem_class) << " at address " << player << " local player is " << player;
#endif
					Vector3 screen;
					Vector3 pos = character.get_position();
					
					if (pos.is_zero() || !unity::world_to_screen(pos, screen))
						continue;

					auto top = character.get_top_point();
					auto name = character.get_hover_name();
					auto health = character.get_health();
					auto max_health = character.get_max_health();
					auto distance = local_player_pos.distance_in_meters(pos);

					if (joaat(classname) == "Player"_hash)
					{
						player p(character.get_object());
						name = p.get_player_name();
					}
					
					char buffer[256];
					snprintf(buffer, sizeof(buffer), "%s [%.2f]m", name.c_str(), distance);

					back.emplace_back(esp_data{
						character == self,
						pos,
						screen,
						distance,
						buffer,
						health,
						max_health,
						top,
						pos,
						joaat(classname) == "Player"_hash ? EEntityType::Player : EEntityType::Character
					});
				}

				g_esp_data.publish();
			}  EXCEPT_CLAUSE

			script::get_current()->yield(30ms);
		}
	}
} // namespace big
