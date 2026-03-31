#include "../view.hpp"
#include "../submenu.hpp"
#include "fiber_pool.hpp"
#include "utility/unity.hpp"
#include "input/input_service.hpp"
#include <custom_teleport/custom_teleport_service.hpp>

namespace big
{
	std::string category = "Default";

	void view::teleport_submenu()
	{
		canvas::add_tab<regular_submenu>("Teleport", SubmenuTeleport, [](regular_submenu* sub)
		{
			sub->add_option<reguler_option>("Teleport Forward", nullptr, [] {
				//player::teleport_forward();
			});
			sub->add_option<sub_option>("Custom Teleport", nullptr, SubmenuCustomTeleport);
		});

		canvas::add_submenu<regular_submenu>("Custom Teleport", SubmenuCustomTeleport, [](regular_submenu* sub)
		{
			g_custom_teleport_service.fetch_saved_locations();
			sub->add_option<reguler_option>("Add Category", nullptr, [] {
				g_input_service.show("Input Category Name", [](std::string const& input) {
					teleport_location new_location;

					auto player = unity::get_local_player();

					if (!player)
						return;

					auto coords = unity::get_position(player);
					auto rotator = unity::get_rotation(player);

					new_location.name = input;
					new_location.x = coords.x;
					new_location.y = coords.y;
					new_location.z = coords.z;
					new_location.rot_x = rotator.x;
					new_location.rot_y = rotator.y;
					new_location.rot_z = rotator.z;
					new_location.rot_w = rotator.w;

					g_custom_teleport_service.save_new_location(input, new_location);
				});
			});

			for (auto& l : g_custom_teleport_service.all_saved_locations | std::ranges::views::keys)
			{
				sub->add_option<sub_option>(l.c_str(), nullptr, joaat(l), [=] {
					category = l;
				});

				canvas::add_submenu<regular_submenu>(l.c_str(), joaat(l), [l](regular_submenu* sub) {
					std::vector<teleport_location> current_list{};
					current_list = g_custom_teleport_service.all_saved_locations.at(l);

					sub->add_option<reguler_option>("Add Teleport", nullptr, [l] {
						g_input_service.show("Input Location Name", [](std::string const& input) {
							teleport_location new_location;

							auto player = unity::get_local_player();

							if (!player)
								return;

							auto coords = unity::get_position(player);
							auto rotator = unity::get_rotation(player);

							new_location.name = input;
							new_location.x = coords.x;
							new_location.y = coords.y;
							new_location.z = coords.z;
							new_location.rot_x = rotator.x;
							new_location.rot_y = rotator.y;
							new_location.rot_z = rotator.z;
							new_location.rot_w = rotator.w;

							g_custom_teleport_service.save_new_location(category, new_location);
						});
					});

					for (const auto& location : current_list)
					{
						sub->add_option<reguler_option>(location.name.c_str(), nullptr, [=] {
							g_fiber_pool->queue_job([=] {
								unity::teleport_to(Vector3(location.x, location.y + 3.f, location.z), Vector4(location.rot_x, location.rot_y, location.rot_z, location.rot_w), true);
							});
						});
					}
				});
			}
		});
	}
}