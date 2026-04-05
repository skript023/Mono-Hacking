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

	static std::unordered_map<uintptr_t, std::string> cache;
	static std::unordered_map<uintptr_t, std::string> new_cache;

    void entity_worker::run()
	{
		while (g_running)
		{
			TRY_CLAUSE
			{
				if (_esp_enabled.get_state())
				{
					auto& back = g_esp_data.back(); back.clear();

					auto self = self::get_player();
					auto characters = character::get_all_scharacters();
					auto local_player_pos = self.get_position();

					for (auto character : characters)
					{
						player p(character.get_object());

						auto obj = (uintptr_t)character.get_object();
						if (!character || obj < 0x10000)
							continue;

						//auto classname = mono::get_name(character.get_object());
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

						Vector3 screen;
						if (!unity::world_to_screen(pos, screen))
							continue;

						auto top = p.get_top_point();
						auto health = p.get_health();
						auto max_health = p.get_max_health();
						auto distance = local_player_pos.distance_in_meters(pos);

						std::string name;

						auto it = cache.find(obj);

						if (it != cache.end())
						{
							name = it->second;
						}
						else
						{
							if (p.is_player())
							{
								name = p.get_player_name();
								LOG(INFO) << "Retreive player name " << name;
							}
							else
							{
								name = p.get_hover_name();

								LOG(INFO) << "Retreive chara name " << name;
							}
						}

						new_cache[obj] = name;

						char buffer[256];
						snprintf(buffer, sizeof(buffer), "%s [%.2f]m", new_cache[obj].c_str(), distance);
						
						back.emplace_back(esp_data{
							false,//character == self
							pos,
							screen,
							distance,
							buffer,
							health,
							max_health,
							top,
							pos,
							p.is_player() ? EEntityType::Player : EEntityType::Character
						});
					}

					cache.swap(new_cache);
					new_cache.clear();

					g_esp_data.publish();
				}
			}  EXCEPT_CLAUSE

			script::get_current()->yield(30ms);
		}

		g_esp_data.clear_all();
	}
} // namespace big
