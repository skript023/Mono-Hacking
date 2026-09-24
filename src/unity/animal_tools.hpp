#pragma once
#include <string>
#include "mono/mono.hpp"
#include "class/vector.hpp"

namespace big::animal_tools
{
	struct creature_info
	{
		bool valid{false};
		MonoObject* character{nullptr};
		std::string name;
		int level{1};
		int stars{0};
		float health{0.f};
		float max_health{0.f};
		bool is_tamed{false};
		bool has_tameable{false};
		int tameness{0};
		float remaining_time{0.f};
		bool is_hungry{false};
		bool is_alerted{false};
		bool has_procreation{false};
		bool is_pregnant{false};
		int love_points{0};
		int required_love_points{4};
		float distance{0.f};
		Vector3 position{};
	};

	struct options
	{
		bool show_inspector{true};
		bool enable_hotkey{true};
		int hotkey{'T'};
		bool heal_on_tame{true};
		float area_tame_radius{30.f};
	};

	void set_options(const options& opt);
	options get_options();

	void update();
	creature_info get_snapshot();
	bool tame_aimed_creature();
	bool tame_creature(MonoObject* creature);
	bool heal_aimed_creature();
	bool heal_creature(MonoObject* creature);
	int tame_all_in_radius(float radius);
	void hotkey_tick();
	void draw_overlay();
}
