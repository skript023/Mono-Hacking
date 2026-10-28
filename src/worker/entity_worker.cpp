#include "entity_worker.hpp"
#include "unity/esp_projection.hpp"

#include "utility/joaat.hpp"
#include "utility/unity.hpp"

#include "unity/self.hpp"
#include "unity/item_drop.hpp"
#include "unity/localization.hpp"

#include "features/features.hpp"
#include "logger/exception_handler.hpp"

#include <chrono>
#include <unordered_map>

namespace big
{
	using namespace features;

	struct cached_label
	{
		std::string text;
		std::chrono::steady_clock::time_point refreshed{};
		uint32_t frame{};
	};

	static std::unordered_map<MonoObject*, cached_label> g_labels;
	static uint32_t g_label_frame = 0;

	template<typename Read>
	static const std::string& cached_text(MonoObject* object, Read&& read)
	{
		const auto now = std::chrono::steady_clock::now();
		auto& label    = g_labels[object];
		label.frame    = g_label_frame;
		if (label.refreshed == std::chrono::steady_clock::time_point{} || now - label.refreshed >= std::chrono::seconds(1))
		{
			label.text      = read();
			label.refreshed = now;
		}
		return label.text;
	}

	static void safe_update_esp(bool& had_esp_data)
	{
		if (_esp_enabled.get_state())
		{
			had_esp_data = true;
			++g_label_frame;
			auto& back = g_esp_data.back();
			back.clear();

			auto self = self::get_player();
			if (!self || !esp_projection::begin_frame())
			{
				g_esp_data.publish();
				return;
			}
			auto characters       = character::get_all_scharacters();
			auto local_player_pos = self.get_position();

			for (auto character : characters)
			{
				auto obj = (uintptr_t)character.get_object();
				if (!character || obj < 0x10000 || character == self)
				{
					continue;
				}

				player p(character.get_object());

				auto classname = mono::get_name(character.get_object());
#ifdef _DEBUG
				MonoClass* elem_class = mono::object_get_class(p.get_object());

				LOG(INFO) << "Element class: " << mono::class_get_namespace(elem_class) << "::" << mono::class_get_name(elem_class) << " at address " << p.get_object() << " local player is " << self.get_object();
#endif
				Vector3 pos = character.get_position();

				if (pos.is_zero())
				{
					continue;
				}

				Vector3 screen;
				if (!esp_projection::project(pos, screen))
				{
					continue;
				}

				Vector3 top{}, top_screen{};
				bool top_visible = false;
				if (_draw_box.get_state() || _draw_health.get_state() || _draw_name.get_state() || _esp_distance.get_state())
				{
					top         = character.get_top_point();
					top_visible = esp_projection::project(top, top_screen);
				}
				auto health     = _draw_health.get_state() ? character.get_health() : 0.f;
				auto max_health = _draw_health.get_state() ? character.get_max_health() : 0.f;
				auto distance   = local_player_pos.distance(pos);

				std::string name;

				if (_draw_name.get_state() || _esp_distance.get_state())
				{
					name = cached_text(character.get_object(), [&] {
						return joaat(classname) == "Player"_hash ? p.get_player_name() : character.get_hover_name();
					});
				}

				char buffer[256]{};
				if (_draw_name.get_state() || _esp_distance.get_state())
				{
					if (_esp_distance.get_state())
					{
						snprintf(buffer, sizeof(buffer), "%s [%.0fm]", name.c_str(), distance);
					}
					else
					{
						snprintf(buffer, sizeof(buffer), "%s", name.c_str());
					}
				}

				back.emplace_back(esp_data{character == self, pos, screen, distance, buffer, health, max_health, top, pos, joaat(classname) == "Player"_hash ? EEntityType::Player : EEntityType::Character, top_screen, top_visible});
			}

			if (_esp_dropped_items.get_state())
			{
				int added_drops = 0;
				for (auto drop : item_drop::get_drops())
				{
					if (added_drops >= 128)
					{
						break;
					}
					if (!drop || !drop.get_object())
					{
						continue;
					}

					auto pos = drop.get_position();
					if (!pos || pos->is_zero())
					{
						continue;
					}

					float distance = local_player_pos.distance(*pos);
					if (distance > 60.f)
					{
						continue;
					}

					Vector3 screen{};
					if (!esp_projection::project(*pos, screen))
					{
						continue;
					}

					Vector3 top = drop.get_bounds_top().value_or(*pos + Vector3{0.f, 0.5f, 0.f});
					Vector3 top_screen{};
					bool top_visible  = esp_projection::project(top, top_screen);
					item_data item    = drop.get_data();
					std::string label = cached_text(drop.get_object(), [&] {
						return localization::get_instance().localize(drop.get_hover_name());
					});
					if (_esp_item_stack.get_state())
					{
						label += std::format(" x{}", item.get_stack());
					}
					if (_esp_distance.get_state())
					{
						label += std::format(" [{:.0f}m]", distance);
					}

					back.emplace_back(esp_data{false, *pos, screen, distance, std::move(label), 0.f, 0.f, top, *pos, EEntityType::ItemDrop, top_screen, top_visible});
					++added_drops;
				}
			}

			std::erase_if(g_labels, [](const auto& entry) {
				return entry.second.frame != g_label_frame;
			});
			g_esp_data.publish();
		}
		else if (had_esp_data)
		{
			g_esp_data.clear_all();
			g_labels.clear();
			had_esp_data = false;
		}
	}

	task<void> entity_worker::run()
	{
		static bool had_esp_data = false;
		while (g_running)
		{
			safe_update_esp(had_esp_data);
			co_await this_task::wait();
		}

		g_esp_data.clear_all();
		co_return;
	}

} // namespace big
