#include "creature_spawner.hpp"

#include "character.hpp"
#include "monster_ai.hpp"
#include "tameable.hpp"
#include "utility/unity.hpp"
#include "task_mgr.hpp"
#include <astra/host/notification.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <astra/components.hpp>
#include <astra/host/canvas.hpp>
#include <format>
#include <imgui.h>
#include <unordered_set>

namespace big
{
	namespace
	{
		std::mutex s_creatures_mutex;
		std::vector<creature_spawner::entry> s_creatures;
		std::atomic<bool> s_scanning{false};
		bool s_window_open = false;
		char s_search[96]{};
		int s_selected   = -1;
		int s_count      = 1;
		int s_level      = 1;
		float s_distance = 5.f;
		int s_placement  = 0;
		bool s_tamed     = false;
		char s_pet_name[64]{};

		Vector3 crosshair_position(MonoObject* player)
		{
			const Vector3 fallback = unity::get_position(player) + unity::get_camera_forward() * 8.f + Vector3{0.f, 0.25f, 0.f};
			static auto get_main = mono::get_method("Camera", "get_main", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto get_transform = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto raycast = mono::get_method_exact("Physics", "Raycast", {"UnityEngine.Vector3", "UnityEngine.Vector3", "UnityEngine.RaycastHit&", "System.Single"}, "UnityEngine.PhysicsModule", "UnityEngine");
			static auto hit_class       = mono::get_class("RaycastHit", "UnityEngine.PhysicsModule", "UnityEngine");
			static auto hit_point_field = hit_class ? mono::get_field(hit_class, "m_Point") : nullptr;
			if (!get_main || !get_transform || !get_position || !raycast || !hit_class || !hit_point_field)
			{
				return fallback;
			}

			auto camera        = mono::invoke_method(get_main, nullptr, nullptr);
			auto transform     = camera ? mono::invoke_method(get_transform, camera, nullptr) : nullptr;
			auto origin_object = transform ? mono::invoke_method(get_position, transform, nullptr) : nullptr;
			auto hit_object    = mono::object_new(hit_class);
			if (!origin_object || !hit_object)
			{
				return fallback;
			}

			Vector3 origin    = *reinterpret_cast<Vector3*>(mono::object_unbox(origin_object));
			Vector3 direction = unity::get_camera_forward().normalize();
			if (direction.length() < 0.1f)
			{
				return fallback;
			}
			origin             = origin + direction * 0.3f;
			float max_distance = 1000.f;
			void* args[]       = {&origin, &direction, mono::object_unbox(hit_object), &max_distance};
			auto did_hit       = mono::invoke_method(raycast, nullptr, args);
			if (!did_hit || !*reinterpret_cast<bool*>(mono::object_unbox(did_hit)))
			{
				return fallback;
			}

			Vector3 point{};
			mono::get_field_value(hit_object, hit_point_field, &point);
			return point + Vector3{0.f, 0.25f, 0.f};
		}

		std::vector<creature_spawner::entry> get_catalog()
		{
			std::lock_guard lock(s_creatures_mutex);
			return s_creatures;
		}
	}

	void creature_spawner::toggle_window()
	{
		s_window_open = !s_window_open;
		if (s_window_open && get_catalog().empty())
		{
			refresh();
		}
	}

	void creature_spawner::refresh()
	{
		if (s_scanning.exchange(true))
		{
			return;
		}
		async_run([] {
			scan_worker();
		});
	}

	void creature_spawner::scan_worker()
	{
		try
		{
			auto scene              = unity::get_znet_scene();
			auto scene_class        = scene ? mono::object_get_class(scene) : nullptr;
			auto prefab_field       = scene_class ? mono::get_field(scene_class, "m_prefabs") : nullptr;
			MonoObject* prefab_list = nullptr;
			if (scene && prefab_field)
			{
				mono::get_field_value(scene, prefab_field, &prefab_list);
			}

			auto character_class = mono::get_class("Character", "assembly_valheim");
			auto character_type  = character_class ? mono::reflection_type(character_class) : nullptr;
			auto player_class    = mono::get_class("Player", "assembly_valheim");
			auto player_type     = player_class ? mono::reflection_type(player_class) : nullptr;
			static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (!get_component)
			{
				get_component = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			}

			std::vector<entry> found;
			std::unordered_set<std::string> seen;
			if (prefab_list && character_type && get_component)
			{
				for (auto* prefab : unity::list_to_vector(prefab_list))
				{
					if (!prefab)
					{
						continue;
					}
					void* args[1] = {character_type};
					auto creature = mono::invoke_method(get_component, prefab, args);
					if (!creature)
					{
						continue;
					}
					if (player_type)
					{
						void* player_args[1] = {player_type};
						if (mono::invoke_method(get_component, prefab, player_args))
						{
							continue;
						}
					}
					std::string name = unity::get_name(prefab);
					if (name.empty() || !seen.insert(name).second)
					{
						continue;
					}
					found.push_back({name, name});
				}
			}
			const auto found_count = found.size();
			std::sort(found.begin(), found.end(), [](const entry& a, const entry& b) {
				return a.name < b.name;
			});
			{
				std::lock_guard lock(s_creatures_mutex);
				s_creatures = std::move(found);
			}
			if (found_count == 0)
			{
				notification::warning("Creature Spawner", "No creature prefabs found. Join a world and refresh the browser.");
			}
			else
			{
				notification::success("Creature Spawner", std::format("Loaded {} creature prefabs from ZNetScene.", found_count));
			}
		}
		catch (...)
		{
			notification::warning("Creature Spawner", "Could not read ZNetScene creature prefabs.");
		}
		s_scanning = false;
	}

	bool creature_spawner::spawn(const std::string& prefab, int count, int level, float distance, int placement, bool tamed, const std::string& pet_name)
	{
		count     = std::clamp(count, 1, 50);
		level     = std::clamp(level, 1, 1000);
		placement = std::clamp(placement, 0, 2);
		async_run([prefab, count, level, distance, placement, tamed, pet_name]() -> task<void> {
			for (int i = 0; i < 30 && unity::get_local_player() && !unity::get_znet_scene(); ++i)
			{
				co_await this_task::wait(std::chrono::milliseconds(100));
			}
			spawn_worker(prefab, count, level, distance, placement, tamed, pet_name);
		});
		return true;
	}

	void creature_spawner::spawn_worker(std::string prefab_name, int count, int level, float distance, int placement, bool tamed, std::string pet_name)
	{
		auto player = unity::get_local_player();
		auto prefab = unity::get_znet_prefab(prefab_name);
		if (!player || !prefab)
		{
			notification::warning("Creature Spawner", "Player or creature prefab is unavailable in this world.");
			return;
		}

		auto scene                = unity::get_znet_scene();
		auto zdo_manager          = mono::get_static_field_value("ZDOMan", "s_instance", "assembly_valheim");
		static auto get_hash      = mono::get_method("ZNetScene", "GetPrefabHash", 1, "assembly_valheim");
		static auto new_zdo       = mono::get_method("ZDOMan", "CreateNewZDO", 2, "assembly_valheim");
		static auto set_prefab    = mono::get_method("ZDO", "SetPrefab", 1, "assembly_valheim");
		static auto set_rotation  = mono::get_method("ZDO", "SetRotation", 1, "assembly_valheim");
		static auto create_object = mono::get_method("ZNetScene", "CreateObject", 1, "assembly_valheim");
		if (!scene || !zdo_manager || !get_hash || !new_zdo || !set_prefab || !set_rotation || !create_object)
		{
			notification::error("Creature Spawner", "Valheim's networked ZDO spawn methods are unavailable.");
			return;
		}
		void* hash_args[1] = {prefab};
		auto hash_result   = mono::invoke_method(get_hash, scene, hash_args);
		if (!hash_result || !mono::object_unbox(hash_result))
		{
			notification::error("Creature Spawner", "Could not read the creature's network prefab ID.");
			return;
		}
		int prefab_hash = *static_cast<int*>(mono::object_unbox(hash_result));
		if (!prefab_hash)
		{
			notification::error("Creature Spawner", "The selected creature is not registered as a network prefab.");
			return;
		}

		Vector3 origin  = unity::get_position(player);
		Vector3 forward = unity::get_forward(player);
		if (forward.length() < 0.1f)
		{
			forward = Vector3{0.f, 0.f, 1.f};
		}
		Quaternions rotation = unity::get_rotation(player);
		int spawned_count    = 0;
		int named_count      = 0;
		for (int i = 0; i < std::clamp(count, 1, 50); ++i)
		{
			Vector3 position = origin;
			if (placement == 0)
			{
				position = position + forward * std::max(1.f, distance);
			}
			else if (placement == 1)
			{
				float angle = 6.2831853f * static_cast<float>(i) / std::max(1, count);
				position    = position + Vector3{std::cos(angle) * distance, 0.f, std::sin(angle) * distance};
			}
			else
			{
				position = crosshair_position(player);
				if (i > 0)
				{
					const float angle = 6.2831853f * static_cast<float>(i) / std::max(1, count);
					position          = position + Vector3{std::cos(angle) * 1.25f, 0.f, std::sin(angle) * 1.25f};
				}
			}
			if (placement != 2)
			{
				position.y += 0.25f;
			}

			std::unordered_set<MonoObject*> previous_characters;
			for (auto current : character::get_all_characters())
			{
				if (auto* object = current.get_object())
				{
					previous_characters.insert(object);
				}
			}

			void* create_args[2] = {&position, &prefab_hash};
			auto zdo             = mono::invoke_method(new_zdo, zdo_manager, create_args);
			if (!zdo)
			{
				continue;
			}
			void* prefab_args[1] = {&prefab_hash};
			mono::invoke_method(set_prefab, zdo, prefab_args);
			void* rotation_args[1] = {&rotation};
			mono::invoke_method(set_rotation, zdo, rotation_args);
			void* object_args[1] = {zdo};
			if (!mono::invoke_method(create_object, scene, object_args))
			{
				continue;
			}

			MonoObject* spawned = nullptr;
			float closest       = 1.5f;
			for (auto current : character::get_all_characters())
			{
				auto* object = current.get_object();
				if (!object || previous_characters.contains(object))
				{
					continue;
				}
				const float candidate_distance = current.get_position().distance(position);
				if (candidate_distance < closest)
				{
					closest = candidate_distance;
					spawned = object;
				}
			}
			if (!spawned)
			{
				continue;
			}
			character creature(spawned);
			creature.set_level(level);
			if (tamed)
			{
				auto tameable = creature.get_tameable();
				if (tameable)
				{
					tameable.tame();
				}
				else
				{
					creature.set_tamed(true);
				}
				auto ai = creature.get_monster_ai();
				if (ai)
				{
					ai.make_tame();
					ai.set_alerted(false);
				}
				if (!pet_name.empty() && tameable && tameable.set_text(pet_name))
				{
					++named_count;
				}
			}
			++spawned_count;
		}
		const std::string name_status = pet_name.empty() ? std::string{} : std::format(" Named {}.", named_count);
		notification::success("Creature Spawner", std::format("Spawned {} of {} at level {}.{}", spawned_count, prefab_name, level, name_status));
	}

	void creature_spawner::draw_window()
	{
		if (!s_window_open)
		{
			return;
		}

		if (!canvas::begin_panel("Creature Spawner", &s_window_open, {.size = {620.f, 500.f}}))
		{
			canvas::end_panel();
			return;
		}
		ImGui::InputTextWithHint("##CreatureSearch", "Search creature prefab", s_search, sizeof(s_search));
		ImGui::SameLine();
		if (ImGui::Button(s_scanning ? "Scanning..." : "Refresh"))
		{
			refresh();
		}

		auto catalog      = get_catalog();
		std::string query = s_search;
		std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		std::vector<int> filtered;
		for (int i = 0; i < static_cast<int>(catalog.size()); ++i)
		{
			std::string name = catalog[i].name;
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (query.empty() || name.find(query) != std::string::npos)
			{
				filtered.push_back(i);
			}
		}
		ImGui::Text("Matches: %zu", filtered.size());
		ImGui::BeginChild("CreatureList", {0.f, 230.f}, true);
		for (int index : filtered)
		{
			ImGui::PushID(catalog[index].prefab.c_str());
			if (ImGui::Selectable(catalog[index].name.c_str(), s_selected == index))
			{
				s_selected = index;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		if (s_selected >= 0 && s_selected < static_cast<int>(catalog.size()))
		{
			astra::slider("Count", &s_count, 1, 50);
			astra::slider("Level", &s_level, 1, 1000);
			astra::slider("Spawn distance", &s_distance, 1.f, 50.f, 0, " m");
			const char* placements[] = {"By distance", "Around player", "At crosshair"};
			astra::choice("Placement", &s_placement, placements);
			ImGui::Checkbox("Spawn tamed", &s_tamed);
			if (s_tamed)
			{
				ImGui::InputTextWithHint("Pet name", "Optional name for spawned pets", s_pet_name, sizeof(s_pet_name));
			}
			if (ImGui::Button("Spawn Creatures", {-1.f, 0.f}))
			{
				spawn(catalog[s_selected].prefab, s_count, s_level, s_distance, s_placement, s_tamed, s_pet_name);
			}
		}
		canvas::end_panel();
	}
}
