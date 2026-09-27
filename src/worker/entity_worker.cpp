#include "entity_worker.hpp"
#include "script.hpp"

#include "utility/joaat.hpp"
#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "unity/item_drop.hpp"

#include "features/features.hpp"

namespace big
{
	using namespace features;

	void entity_worker::run()
	{
		bool had_esp_data = false;
		while (g_running)
		{
			TRY_CLAUSE
			{
				if (_esp_enabled.get_state())
				{
					had_esp_data = true;
					auto& back = g_esp_data.back();
					back.clear();

					auto self = self::get_player();
					auto characters = character::get_all_scharacters();
					auto local_player_pos = self.get_position();

					for (auto character : characters)
					{
						auto obj = (uintptr_t)character.get_object();
						if (!character || obj < 0x10000 || character == self)
							continue;

						player p(character.get_object());

						auto classname = mono::get_name(character.get_object());
#ifdef _DEBUG
						MonoClass* elem_class = mono::object_get_class(p.get_object());

						LOG(INFO) << "Element class: "
						          << mono::class_get_namespace(elem_class)
						          << "::"
						          << mono::class_get_name(elem_class) << " at address " << p.get_object()
						          << " local player is " << self.get_object();
#endif
						Vector3 pos = character.get_position();

						if (pos.is_zero())
							continue;

						Vector3 screen;
						if (!unity::world_to_screen(pos, screen))
							continue;

						Vector3 top{}, top_screen{};
						bool top_visible = false;
						if (_draw_box.get_state() || _draw_health.get_state())
						{
							top = character.get_top_point();
							top_visible = unity::world_to_screen(top, top_screen);
						}
						auto health = _draw_health.get_state() ? character.get_health() : 0.f;
						auto max_health = _draw_health.get_state() ? character.get_max_health() : 0.f;
						auto distance = local_player_pos.distance_in_meters(pos);

						std::string name;

						if (_draw_name.get_state() && joaat(classname) == "Player"_hash)
						{
							name = p.get_player_name();
						}
						else if (_draw_name.get_state())
						{
							name = character.get_hover_name();
						}

						char buffer[256]{};
						if (_draw_name.get_state())
							snprintf(buffer, sizeof(buffer), "%s [%.2f]m", name.c_str(), distance);

						back.emplace_back(esp_data{
						    character == self, //character == self
						    pos,
						    screen,
						    distance,
						    buffer,
						    health,
						    max_health,
						    top,
						    pos,
						    joaat(classname) == "Player"_hash ? EEntityType::Player : EEntityType::Character,
						    top_screen,
						    top_visible});
					}

					g_esp_data.publish();
				}
				else if (had_esp_data)
				{
					g_esp_data.clear_all();
					had_esp_data = false;
				}
			}
			EXCEPT_CLAUSE

			script::get_current()->yield(30ms);
		}

		g_esp_data.clear_all();
	}

} // namespace big
