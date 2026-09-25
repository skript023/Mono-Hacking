#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include "character.hpp"
#include "class/vector.hpp"
#include "mono/mono.hpp"
#include "monster_ai.hpp"
#include "procreation.hpp"
#include "tameable.hpp"
#include <string>

namespace big
{
	class animal_tools
	{
	public:
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

		static void set_options(const options& opt)
		{
			return instance().set_options_impl(opt);
		}
		static options get_options()
		{
			return instance().get_options_impl();
		}

		static void update()
		{
			return instance().update_impl();
		}
		static creature_info get_snapshot()
		{
			return instance().get_snapshot_impl();
		}
		static bool tame_aimed_creature()
		{
			return instance().tame_aimed_creature_impl();
		}
		static bool tame_creature(character creature)
		{
			return instance().tame_creature_impl(creature);
		}
		static bool tame_creature(MonoObject* creature)
		{
			return instance().tame_creature_impl(creature);
		}
		static bool heal_aimed_creature()
		{
			return instance().heal_aimed_creature_impl();
		}
		static bool heal_creature(character creature)
		{
			return instance().heal_creature_impl(creature);
		}
		static bool heal_creature(MonoObject* creature)
		{
			return instance().heal_creature_impl(creature);
		}
		static int tame_all_in_radius(float radius)
		{
			return instance().tame_all_in_radius_impl(radius);
		}
		static void hotkey_tick()
		{
			return instance().hotkey_tick_impl();
		}
		static void draw_overlay()
		{
			return instance().draw_overlay_impl();
		}

	private:
		animal_tools() = default;
		animal_tools(const animal_tools&) = delete;
		animal_tools& operator=(const animal_tools&) = delete;
		static animal_tools& instance()
		{
			static animal_tools value;
			return value;
		}

		void set_options_impl(const options& opt);
		options get_options_impl();
		void update_impl();
		creature_info get_snapshot_impl();
		bool tame_aimed_creature_impl();
		bool tame_creature_impl(character creature);
		bool tame_creature_impl(MonoObject* creature);
		bool heal_aimed_creature_impl();
		bool heal_creature_impl(character creature);
		bool heal_creature_impl(MonoObject* creature);
		int tame_all_in_radius_impl(float radius);
		void hotkey_tick_impl();
		void draw_overlay_impl();

		options g_options;
		std::mutex g_mutex;
		creature_info g_snapshot;
		std::chrono::steady_clock::time_point g_last_seen{};
		static constexpr auto HYSTERESIS_MS = std::chrono::milliseconds(500);
	};
}
