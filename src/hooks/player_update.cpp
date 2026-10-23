#include "hooking.hpp"
#include "logger/exception_handler.hpp"
#include "unity/equipment_slots.hpp"
#include "unity/buff_tools.hpp"
#include "utility/unity.hpp"
#include "unity/character.hpp"

#include <algorithm>
#include <chrono>

namespace big
{
	static void update_attack_speed(MonoObject* player, bool enabled)
	{
		static auto animator_field = mono::get_field("Character", "m_animator", "assembly_valheim");
		static auto get_speed      = mono::get_method("Animator", "get_speed", 0, "UnityEngine.AnimationModule", "UnityEngine");
		static auto set_speed = mono::get_method_overload("Animator", "set_speed", 1, "System.Void", "System.Single", "UnityEngine.AnimationModule", "UnityEngine");
		static auto in_attack = mono::get_method("Humanoid", "InAttack", 0, "assembly_valheim");
		static MonoObject* scaled_animator = nullptr;
		static float scaled_speed          = 1.f;
		static float game_speed            = 1.f;

		if (!animator_field || !get_speed || !set_speed || !in_attack)
		{
			return;
		}

		const float multiplier = std::clamp(g_settings.self.attack_animation_speed, 0.5f, 5.f);
		const bool scaling     = enabled && multiplier != 1.f;
		MonoObject* animator   = nullptr;
		mono::get_field_value(player, animator_field, &animator);
		if (animator != scaled_animator)
		{
			scaled_animator = nullptr;
		}
		if (!animator || (!scaling && !scaled_animator))
		{
			return;
		}

		MonoObject* exception = nullptr;
		auto boxed_speed      = mono::invoke_method(get_speed, animator, nullptr, &exception);
		auto speed_value      = boxed_speed ? mono::object_unbox(boxed_speed) : nullptr;
		if (exception || !speed_value)
		{
			return;
		}
		const float current = *reinterpret_cast<float*>(speed_value);
		const float vanilla = scaled_animator && current == scaled_speed ? game_speed : current;

		float target = vanilla;
		if (scaling && vanilla >= 0.01f)
		{
			auto attacking       = mono::invoke_method(in_attack, player, nullptr, &exception);
			auto attacking_value = attacking ? mono::object_unbox(attacking) : nullptr;
			if (!exception && attacking_value && *reinterpret_cast<bool*>(attacking_value))
			{
				target = vanilla * multiplier;
			}
		}

		if (target != current)
		{
			void* args[] = {&target};
			mono::invoke_method(set_speed, animator, args, &exception);
			if (exception)
			{
				return;
			}
		}
		if (target != vanilla)
		{
			scaled_animator = animator;
			scaled_speed    = target;
			game_speed      = vanilla;
		}
		else if (vanilla >= 0.01f)
		{
			scaled_animator = nullptr;
		}
	}

	static void update_noclip(MonoObject* player)
	{
		static auto get_collider = mono::get_method("Player", "GetCollider", 0, "assembly_valheim");
		static auto get_enabled = mono::get_method("Collider", "get_enabled", 0, "UnityEngine.PhysicsModule", "UnityEngine");
		static auto set_enabled = mono::get_method_overload("Collider", "set_enabled", 1, "System.Void", "System.Boolean", "UnityEngine.PhysicsModule", "UnityEngine");
		static uintptr_t active_collider = 0;
		static bool original_enabled     = true;

		const bool noclip = g_running && g_settings.self.noclip;
		if ((!noclip && !active_collider) || !get_collider || !get_enabled || !set_enabled)
		{
			return;
		}

		auto collider = noclip ? mono::invoke_method(get_collider, player, nullptr) : nullptr;
		auto previous = mono::retained_object(active_collider);
		if (active_collider && previous != collider)
		{
			if (previous)
			{
				void* restore_args[]  = {&original_enabled};
				MonoObject* exception = nullptr;
				mono::invoke_method(set_enabled, previous, restore_args, &exception);
			}
			mono::release(active_collider);
			active_collider = 0;
		}
		if (!collider)
		{
			return;
		}

		if (!active_collider)
		{
			MonoObject* exception = nullptr;
			auto enabled          = mono::invoke_method(get_enabled, collider, nullptr, &exception);
			auto value            = enabled ? mono::object_unbox(enabled) : nullptr;
			if (exception || !value)
			{
				return;
			}
			original_enabled = *reinterpret_cast<bool*>(value);
			active_collider  = mono::retain(collider);
		}

		bool enabled          = false;
		void* args[]          = {&enabled};
		MonoObject* exception = nullptr;
		mono::invoke_method(set_enabled, collider, args, &exception);
	}

	static void update_build_overlap(MonoObject* player, bool enabled)
	{
		static auto player_class = mono::get_class("Player", "assembly_valheim");
		static auto ghost_field  = player_class ? mono::get_field(player_class, "m_placementGhost") : nullptr;
		static auto piece_class  = mono::get_class("Piece", "assembly_valheim");
		static auto piece_type   = piece_class ? mono::reflection_type(piece_class) : nullptr;
		static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		static auto overlap_field       = piece_class ? mono::get_field(piece_class, "m_allowRotatedOverlap") : nullptr;
		static MonoObject* cached_piece = nullptr;
		static bool original_overlap    = false;
		if (!player || !ghost_field || !piece_type || !get_component || !overlap_field)
		{
			return;
		}

		MonoObject* ghost = nullptr;
		mono::get_field_value(player, ghost_field, &ghost);
		void* args[] = {piece_type};
		auto piece   = ghost ? mono::invoke_method(get_component, ghost, args) : nullptr;
		if (!piece)
		{
			return;
		}
		if (piece != cached_piece)
		{
			cached_piece = piece;
			mono::get_field_value(piece, overlap_field, &original_overlap);
		}

		bool allow_overlap = original_overlap || (enabled && g_settings.self.allow_rotated_piece_overlap);
		mono::set_field_value(piece, overlap_field, &allow_overlap);
	}

	void hooks::update(MonoObject* player)
	{
		auto local_player                     = unity::get_local_player();
		static MonoObject* cached_player      = nullptr;
		static float base_swim_speed          = 2.f;
		static float base_jump_force          = 10.f;
		static float base_pickup_range        = 2.f;
		static float base_build_distance      = 5.f;
		static float base_rotation_step       = 10.f;
		static float base_place_delay         = 0.1f;
		static float base_remove_delay        = 0.1f;
		static MonoObject* last_pickup_player = nullptr;
		static float last_pickup_range        = 2.f;
		static auto last_tick                 = std::chrono::steady_clock::now();

		if (local_player && player == local_player && cached_player != local_player)
		{
			cached_player       = local_player;
			base_swim_speed     = mono::get_field_value<"Character", "m_swimSpeed", float>(local_player);
			base_jump_force     = mono::get_field_value<"Character", "m_jumpForce", float>(local_player);
			base_pickup_range   = mono::get_field_value<"Player", "m_autoPickupRange", float>(local_player);
			base_build_distance = mono::get_field_value<"Player", "m_maxPlaceDistance", float>(local_player);
			base_rotation_step  = mono::get_field_value<"Player", "m_placeRotationDegrees", float>(local_player);
			base_place_delay    = mono::get_field_value<"Player", "m_placeDelay", float>(local_player);
			base_remove_delay   = mono::get_field_value<"Player", "m_removeDelay", float>(local_player);
			last_tick           = std::chrono::steady_clock::now();
		}

		if (local_player && player == local_player)
		{
			update_noclip(local_player);
			update_build_overlap(local_player, g_running);
			update_attack_speed(local_player, g_running);
		}

		if (g_running && local_player && player == local_player)
		{
			equipment_manager::get().tick(player);

			if (last_pickup_player != local_player || last_pickup_range != g_settings.self.pickup_range)
			{
				last_pickup_player = local_player;
				last_pickup_range  = g_settings.self.pickup_range;
				mono::set_field_value<"Player", "m_autoPickupRange">(local_player, g_settings.self.pickup_range);
			}

			mono::set_field_value<"Character", "m_swimSpeed">(local_player, base_swim_speed * std::clamp(g_settings.self.swim_speed_multiplier, 0.1f, 20.f));
			mono::set_field_value<"Character", "m_jumpForce">(local_player, base_jump_force * std::clamp(g_settings.self.jump_force_multiplier, 0.1f, 20.f));
			float build_distance = std::clamp(g_settings.self.build_distance, 5.f, 50.f);
			float rotation_step  = std::clamp(g_settings.self.build_rotation_step, 1.f, 22.5f);
			float place_delay    = std::clamp(g_settings.self.build_place_delay, 0.01f, 2.f);
			float remove_delay   = std::clamp(g_settings.self.build_remove_delay, 0.01f, 2.f);
			mono::set_field_value<"Player", "m_maxPlaceDistance">(local_player, build_distance);
			mono::set_field_value<"Player", "m_placeRotationDegrees">(local_player, rotation_step);
			mono::set_field_value<"Player", "m_placeDelay">(local_player, place_delay);
			mono::set_field_value<"Player", "m_removeDelay">(local_player, remove_delay);
		}
		else if (local_player && player == local_player)
		{
			mono::set_field_value<"Character", "m_swimSpeed">(local_player, base_swim_speed);
			mono::set_field_value<"Character", "m_jumpForce">(local_player, base_jump_force);
			mono::set_field_value<"Player", "m_autoPickupRange">(local_player, base_pickup_range);
			last_pickup_player = nullptr;
			mono::set_field_value<"Player", "m_maxPlaceDistance">(local_player, base_build_distance);
			mono::set_field_value<"Player", "m_placeRotationDegrees">(local_player, base_rotation_step);
			mono::set_field_value<"Player", "m_placeDelay">(local_player, base_place_delay);
			mono::set_field_value<"Player", "m_removeDelay">(local_player, base_remove_delay);
		}

		detour_base::get_original<update>()(player);
		if (g_running && local_player && player == local_player)
		{
			if (g_settings.self.prevent_freezing)
			{
				buff_tools::remove_status_effect("Freezing");
			}
			if (g_settings.self.prevent_cold)
			{
				buff_tools::remove_status_effect("Cold");
			}
			if (g_settings.self.is_wet)
			{
				buff_tools::remove_status_effect("Wet");
			}

			const auto now      = std::chrono::steady_clock::now();
			const float elapsed = std::clamp(std::chrono::duration<float>(now - last_tick).count(), 0.f, 0.25f);
			last_tick           = now;
			character local_character(local_player);
			if (g_settings.self.maintain_health)
			{
				local_character.set_health(local_character.get_max_health());
			}
			else if (g_settings.self.health_regen_rate > 0.f && !local_character.is_dead())
			{
				local_character.set_health(
				    std::min(local_character.get_max_health(), local_character.get_health() + g_settings.self.health_regen_rate * elapsed));
			}
			if (g_settings.self.maintain_stamina)
			{
				static auto get_max_stamina = mono::get_method("Player", "GetMaxStamina", 0, "assembly_valheim");
				auto max_stamina = get_max_stamina ? mono::invoke_method(get_max_stamina, local_player, nullptr) : nullptr;
				if (max_stamina)
				{
					mono::set_field_value<"Player", "m_stamina">(local_player, *reinterpret_cast<float*>(mono::object_unbox(max_stamina)));
				}
			}
			if (g_settings.self.maintain_eitr)
			{
				static auto get_max_eitr = mono::get_method("Player", "GetMaxEitr", 0, "assembly_valheim");
				auto max_eitr = get_max_eitr ? mono::invoke_method(get_max_eitr, local_player, nullptr) : nullptr;
				if (max_eitr)
				{
					mono::set_field_value<"Player", "m_eitr">(local_player, *reinterpret_cast<float*>(mono::object_unbox(max_eitr)));
				}
			}
		}
	}
}
