#include "entity_worker.hpp"
#include "script.hpp"

#include "utility/unity.hpp"

namespace big
{
    void entity_worker::run()
	{
		while (g_running)
		{
			TRY_CLAUSE
			{
				auto& back = g_esp_data.back(); back.clear();

				auto list = unity::get_all_characters();

				for (auto player : list)
				{
					auto local_player = unity::get_local_player();
					if (!player || (uintptr_t)player < 0x10000 || player == local_player)
						continue;

#ifdef _DEBUG
					MonoClass* elem_class = mono::object_get_class(player);

					LOG(INFO) << "Element class: "
						<< mono::class_get_namespace(elem_class)
						<< "::"
						<< mono::class_get_name(elem_class) << " at address " << player << " local player is " << unity::get_local_player();
#endif

					Vector3 pos = unity::get_position(player);
					auto local_player_pos = unity::get_position(local_player);

					if (pos.is_zero())
						continue;

					Vector3 top{};
					Vector3 bottom{};

					auto name = unity::get_hover_name(player);
					auto health = unity::get_health(player);
					auto max_health = unity::get_max_health(player);
					auto distance = local_player_pos.distance(pos);
					
					if (!unity::get_bounds(player, top, bottom))
						continue;

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
					data.bottom = bottom;
					back.push_back(data);
				}

				g_esp_data.publish();
			} 
			EXCEPT_CLAUSE
			script::get_current()->yield(50ms);
		}
	}
} // namespace big
