#include "prefab_spawner.hpp"

#include <astra/host/notification.hpp>
#include "item_spawner.hpp"
#include "task_mgr.hpp"
#include "utility/unity.hpp"
#include "services/mono/array.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <format>
#include <astra/components.hpp>
#include <astra/host/canvas.hpp>
#include <imgui.h>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace big
{
	namespace
	{
		struct prefab_entry
		{
			std::string name;
			std::string category;
		};

		struct spawned_entry
		{
			std::string name;
			uintptr_t object_handle{};
			uintptr_t scene_handle{};
		};

		std::mutex s_prefab_mutex;
		std::vector<prefab_entry> s_prefabs;
		std::vector<spawned_entry> s_history;
		bool s_open = false;
		std::atomic_bool s_scanning{false};
		char s_search[96]{};
		int s_selected = -1;
		int s_count    = 1;
		int s_category = 0;
		Vector3 s_position{};
		Vector3 s_rotation{};
		bool s_position_initialized                       = false;
		constexpr std::array<const char*, 6> s_categories = {"All", "Creatures", "Items", "Building", "World", "Other"};

		void reset_history_for_scene(MonoObject* scene)
		{
			std::lock_guard lock(s_prefab_mutex);
			for (auto it = s_history.begin(); it != s_history.end();)
			{
				if (mono::retained_object(it->scene_handle) != scene)
				{
					mono::release(it->object_handle);
					mono::release(it->scene_handle);
					it = s_history.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		MonoObject* find_new_prefab_instance(MonoObject* prefab, const Vector3& position, const std::unordered_set<MonoObject*>& previous)
		{
			static auto find_objects = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			auto game_object_class = mono::get_class("GameObject", "UnityEngine.CoreModule", "UnityEngine");
			auto game_object_type  = game_object_class ? mono::reflection_type(game_object_class) : nullptr;
			if (!find_objects || !game_object_type)
			{
				return nullptr;
			}
			void* args[1]          = {game_object_type};
			auto candidates        = mono::invoke_method(find_objects, nullptr, args);
			MonoObject* closest    = nullptr;
			float closest_distance = 0.8f;
			if (candidates)
			{
				for (auto* candidate : mono_array_view<MonoObject*>(reinterpret_cast<MonoArray*>(candidates)))
				{
					if (!candidate || previous.contains(candidate))
					{
						continue;
					}
					const auto candidate_name = unity::get_name(candidate);
					const auto prefab_name    = unity::get_name(prefab);
					if (candidate_name != prefab_name && candidate_name != prefab_name + "(Clone)")
					{
						continue;
					}
					const float distance = unity::get_position(candidate).distance(position);
					if (distance < closest_distance)
					{
						closest_distance = distance;
						closest          = candidate;
					}
				}
			}
			return closest;
		}

		void record_spawn(std::string name, MonoObject* object, MonoObject* scene)
		{
			if (!object || !scene)
			{
				return;
			}
			spawned_entry entry{std::move(name), mono::retain(object), mono::retain(scene)};
			std::lock_guard lock(s_prefab_mutex);
			s_history.push_back(std::move(entry));
			if (s_history.size() > 100)
			{
				mono::release(s_history.front().object_handle);
				mono::release(s_history.front().scene_handle);
				s_history.erase(s_history.begin());
			}
		}

		std::string categorize(const std::string& name)
		{
			std::string lower = name;
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (lower.find("troll") != std::string::npos || lower.find("skeleton") != std::string::npos || lower.find("boar") != std::string::npos
			    || lower.find("wolf") != std::string::npos || lower.find("neck") != std::string::npos || lower.find("serpent") != std::string::npos
			    || lower.find("goblin") != std::string::npos || lower.find("golem") != std::string::npos || lower.find("blob") != std::string::npos
			    || lower.find("surtling") != std::string::npos || lower.find("drake") != std::string::npos || lower.find("greydwarf") != std::string::npos
			    || lower.find("lox") != std::string::npos || lower.find("hare") != std::string::npos || lower.find("asksvin") != std::string::npos
			    || lower.find("morgen") != std::string::npos || lower.find("tick") != std::string::npos)
			{
				return "Creatures";
			}
			if (lower.find("tree") != std::string::npos || lower.find("rock") != std::string::npos || lower.find("dungeon") != std::string::npos
			    || lower.find("location") != std::string::npos || lower.find("stone_") != std::string::npos)
			{
				return "World";
			}
			if (lower.find("item") != std::string::npos || lower.find("wood") != std::string::npos || lower.find("ore") != std::string::npos
			    || lower.find("bar") != std::string::npos || lower.find("food") != std::string::npos || lower.find("arrow") != std::string::npos
			    || lower.find("sword") != std::string::npos || lower.find("shield") != std::string::npos)
			{
				return "Items";
			}
			return "Other";
		}

		std::vector<prefab_entry> get_prefabs()
		{
			std::lock_guard lock(s_prefab_mutex);
			return s_prefabs;
		}

		void scan_prefabs()
		{
			if (s_scanning.exchange(true))
			{
				return;
			}
			async_run([] {
				struct scan_reset
				{
					~scan_reset()
					{
						s_scanning = false;
					}
				} reset;
				auto scene        = unity::get_znet_scene();
				auto scene_class  = scene ? mono::object_get_class(scene) : nullptr;
				auto prefab_field = scene_class ? mono::get_field(scene_class, "m_prefabs") : nullptr;
				MonoObject* list  = nullptr;
				if (scene && prefab_field)
				{
					mono::get_field_value(scene, prefab_field, &list);
				}
				auto piece_class     = mono::get_class("Piece", "assembly_valheim");
				auto character_class = mono::get_class("Character", "assembly_valheim");
				auto item_drop_class = mono::get_class("ItemDrop", "assembly_valheim");
				auto piece_type      = piece_class ? mono::reflection_type(piece_class) : nullptr;
				auto character_type  = character_class ? mono::reflection_type(character_class) : nullptr;
				auto item_drop_type  = item_drop_class ? mono::reflection_type(item_drop_class) : nullptr;
				static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				if (!get_component)
				{
					get_component = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				}

				std::vector<prefab_entry> found;
				if (list)
				{
					for (auto* prefab : unity::list_to_vector(list))
					{
						if (!prefab)
						{
							continue;
						}
						auto name = unity::get_name(prefab);
						if (!name.empty())
						{
							bool is_piece     = false;
							bool is_character = false;
							bool is_item_drop = false;
							if (get_component)
							{
								if (piece_type)
								{
									void* args[] = {piece_type};
									is_piece     = mono::invoke_method(get_component, prefab, args) != nullptr;
								}
								if (character_type)
								{
									void* args[] = {character_type};
									is_character = mono::invoke_method(get_component, prefab, args) != nullptr;
								}
								if (item_drop_type)
								{
									void* args[] = {item_drop_type};
									is_item_drop = mono::invoke_method(get_component, prefab, args) != nullptr;
								}
							}
							const std::string category = is_piece ? "Building" :
							    is_character                      ? "Creatures" :
							    is_item_drop                      ? "Items" :
							                                        categorize(name);
							found.push_back({name, category});
						}
					}
				}
				std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
					return a.name < b.name;
				});
				found.erase(std::unique(found.begin(),
				                found.end(),
				                [](const auto& a, const auto& b) {
					                return a.name == b.name;
				                }),
				    found.end());
				{
					std::lock_guard lock(s_prefab_mutex);
					s_prefabs = std::move(found);
				}
				notification::success("Prefab Spawner", std::format("Loaded {} runtime prefabs.", get_prefabs().size()));
			});
		}

		void find_matching_build_tool(std::string piece_name)
		{
			async_run([piece_name = std::move(piece_name)] {
				auto database         = unity::get_object_db();
				auto database_class   = database ? mono::object_get_class(database) : nullptr;
				auto items_field      = database_class ? mono::get_field(database_class, "m_items") : nullptr;
				MonoObject* item_list = nullptr;
				if (database && items_field)
				{
					mono::get_field_value(database, items_field, &item_list);
				}
				if (!item_list)
				{
					notification::warning("Piece Browser", "Item database is unavailable. Join a world and retry.");
					return;
				}

				auto item_class = mono::get_class("ItemDrop", "assembly_valheim");
				auto item_type  = item_class ? mono::reflection_type(item_class) : nullptr;
				static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				if (!get_component)
				{
					get_component = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				}
				auto item_data_field = item_class ? mono::get_field(item_class, "m_itemData") : nullptr;
				auto data_class      = mono::get_class("ItemDrop/ItemData", "assembly_valheim");
				auto shared_field    = data_class ? mono::get_field(data_class, "m_shared") : nullptr;
				auto shared_class    = mono::get_class("ItemDrop/ItemData/SharedData", "assembly_valheim");
				auto table_field     = shared_class ? mono::get_field(shared_class, "m_buildPieces") : nullptr;
				auto table_class     = mono::get_class("PieceTable", "assembly_valheim");
				auto pieces_field    = table_class ? mono::get_field(table_class, "m_pieces") : nullptr;
				if (!item_type || !get_component || !item_data_field || !shared_field || !table_field || !pieces_field)
				{
					notification::warning("Piece Browser", "Runtime build tool metadata is unavailable.");
					return;
				}

				for (auto* prefab : unity::list_to_vector(item_list))
				{
					if (!prefab)
					{
						continue;
					}
					void* args[]          = {item_type};
					auto item_drop        = mono::invoke_method(get_component, prefab, args);
					MonoObject* item_data = nullptr;
					if (item_drop)
					{
						mono::get_field_value(item_drop, item_data_field, &item_data);
					}
					MonoObject* shared = nullptr;
					if (item_data)
					{
						mono::get_field_value(item_data, shared_field, &shared);
					}
					MonoObject* table = nullptr;
					if (shared)
					{
						mono::get_field_value(shared, table_field, &table);
					}
					MonoObject* pieces = nullptr;
					if (table)
					{
						mono::get_field_value(table, pieces_field, &pieces);
					}
					if (!pieces)
					{
						continue;
					}
					auto piece_prefabs = unity::list_to_vector(pieces);
					const auto matches = std::any_of(piece_prefabs.begin(), piece_prefabs.end(), [&](MonoObject* piece) {
						return piece && unity::get_name(piece) == piece_name;
					});
					if (matches)
					{
						const auto tool_name = unity::get_name(prefab);
						item_spawner::focus_item(tool_name);
						notification::success("Piece Browser", std::format("{} is available from {}. Item Spawner opened.", piece_name, tool_name));
						return;
					}
				}
				notification::warning("Piece Browser", std::format("No runtime build tool contains {}.", piece_name));
			});
		}

		void spawn_prefab(std::string prefab_name, int count, Vector3 position, Vector3 euler_rotation)
		{
			async_run([prefab_name = std::move(prefab_name), count, position, euler_rotation] {
				auto scene  = unity::get_znet_scene();
				auto prefab = unity::get_znet_prefab(prefab_name);
				if (!scene || !prefab)
				{
					notification::warning("Prefab Spawner", "The runtime prefab or network scene is unavailable.");
					return;
				}
				static auto spawn_rpc     = mono::get_method("ZNetScene", "SpawnObject", 3, "assembly_valheim");
				static auto get_hash      = mono::get_method("ZNetScene", "GetPrefabHash", 1, "assembly_valheim");
				static auto new_zdo       = mono::get_method("ZDOMan", "CreateNewZDO", 2, "assembly_valheim");
				static auto set_prefab    = mono::get_method("ZDO", "SetPrefab", 1, "assembly_valheim");
				static auto set_rotation  = mono::get_method("ZDO", "SetRotation", 1, "assembly_valheim");
				static auto create_object = mono::get_method("ZNetScene", "CreateObject", 1, "assembly_valheim");
				static auto quaternion_euler = mono::get_method("Quaternion", "Euler", 3, "UnityEngine.CoreModule", "UnityEngine");
				static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				auto zdo_manager       = mono::get_static_field_value("ZDOMan", "s_instance", "assembly_valheim");
				auto znet_view_class   = mono::get_class("ZNetView", "assembly_valheim");
				auto znet_view_type    = znet_view_class ? mono::reflection_type(znet_view_class) : nullptr;
				auto game_object_class = mono::get_class("GameObject", "UnityEngine.CoreModule", "UnityEngine");
				auto game_object_type  = game_object_class ? mono::reflection_type(game_object_class) : nullptr;
				static auto find_objects = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
				if (!spawn_rpc || !get_hash || !new_zdo || !set_prefab || !set_rotation || !create_object || !quaternion_euler || !get_component || !zdo_manager || !znet_view_type)
				{
					notification::error("Prefab Spawner", "Valheim's prefab spawn methods are unavailable.");
					return;
				}
				void* hash_args[1] = {prefab};
				auto hash_result   = mono::invoke_method(get_hash, scene, hash_args);
				if (!hash_result || !mono::object_unbox(hash_result))
				{
					notification::error("Prefab Spawner", "Could not read the selected prefab's network ID.");
					return;
				}
				int prefab_hash          = *static_cast<int*>(mono::object_unbox(hash_result));
				void* component_args[1]  = {znet_view_type};
				const bool has_znet_view = mono::invoke_method(get_component, prefab, component_args) != nullptr;
				float rotation_x         = euler_rotation.x;
				float rotation_y         = euler_rotation.y;
				float rotation_z         = euler_rotation.z;
				void* euler_args[3]      = {&rotation_x, &rotation_y, &rotation_z};
				auto rotation            = mono::invoke_method(quaternion_euler, nullptr, euler_args);
				if (!rotation)
				{
					return;
				}
				reset_history_for_scene(scene);
				int spawned_count = 0;
				for (int i = 0; i < std::clamp(count, 1, 100); ++i)
				{
					Vector3 point = position;
					if (count > 1)
					{
						const float angle = 6.2831853f * static_cast<float>(i) / static_cast<float>(count);
						point.x += std::cos(angle) * 1.25f;
						point.z += std::sin(angle) * 1.25f;
					}
					bool spawned               = false;
					MonoObject* spawned_object = nullptr;
					std::unordered_set<MonoObject*> previous_objects;
					if (!has_znet_view && find_objects && game_object_type)
					{
						void* find_args[1] = {game_object_type};
						auto objects       = mono::invoke_method(find_objects, nullptr, find_args);
						if (objects)
						{
							for (auto* object : mono_array_view<MonoObject*>(reinterpret_cast<MonoArray*>(objects)))
							{
								if (object)
								{
									previous_objects.insert(object);
								}
							}
						}
					}
					if (has_znet_view)
					{
						void* zdo_args[2] = {&point, &prefab_hash};
						auto zdo          = mono::invoke_method(new_zdo, zdo_manager, zdo_args);
						if (zdo)
						{
							void* prefab_args[1] = {&prefab_hash};
							mono::invoke_method(set_prefab, zdo, prefab_args);
							void* rotation_args[1] = {mono::object_unbox(rotation)};
							mono::invoke_method(set_rotation, zdo, rotation_args);
							void* object_args[1] = {zdo};
							spawned_object       = mono::invoke_method(create_object, scene, object_args);
							spawned              = spawned_object != nullptr;
						}
					}
					else
					{
						void* args[3] = {&point, mono::object_unbox(rotation), prefab};
						mono::invoke_method(spawn_rpc, scene, args);
						spawned_object = find_new_prefab_instance(prefab, point, previous_objects);
						spawned        = spawned_object != nullptr;
					}
					if (!spawned)
					{
						continue;
					}
					++spawned_count;
					record_spawn(prefab_name, spawned_object, scene);
				}
				notification::success("Prefab Spawner", std::format("Requested {} x {}.", spawned_count, prefab_name));
			});
		}

		void undo_last()
		{
			async_run([] {
				spawned_entry last;
				{
					std::lock_guard lock(s_prefab_mutex);
					if (s_history.empty())
					{
						return;
					}
					last = s_history.back();
					s_history.pop_back();
				}
				auto scene            = unity::get_znet_scene();
				auto object           = mono::retained_object(last.object_handle);
				static auto destroy   = mono::get_method("ZNetScene", "Destroy", 1, "assembly_valheim");
				const bool same_scene = scene && mono::retained_object(last.scene_handle) == scene;
				if (same_scene && destroy && object)
				{
					void* args[1] = {object};
					mono::invoke_method(destroy, scene, args);
					notification::success("Prefab Spawner", std::format("Removed {}.", last.name));
				}
				else
				{
					notification::warning("Prefab Spawner", "The spawned object is gone or belongs to another world session.");
				}
				mono::release(last.object_handle);
				mono::release(last.scene_handle);
			});
		}
	}

	void prefab_spawner::toggle_window()
	{
		s_open = !s_open;
		if (s_open && get_prefabs().empty())
		{
			scan_prefabs();
		}
	}

	void prefab_spawner::open_piece_browser()
	{
		s_category = 3;
		if (!s_open)
		{
			s_open = true;
		}
		if (get_prefabs().empty())
		{
			scan_prefabs();
		}
	}

	void prefab_spawner::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		if (!canvas::begin_panel("Prefab Spawner", &s_open, {.size = {660.f, 540.f}}))
		{
			canvas::end_panel();
			return;
		}
		ImGui::TextDisabled("Runtime catalog  /  %zu prefabs", get_prefabs().size());
		ImGui::SameLine();
		if (ImGui::SmallButton(s_scanning.load() ? "Scanning" : "Refresh"))
		{
			scan_prefabs();
		}
		astra::choice("Category", &s_category, s_categories);
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##PrefabSearch", "Search runtime prefab", s_search, sizeof(s_search));
		auto prefabs      = get_prefabs();
		std::string query = s_search;
		std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		ImGui::BeginChild("PrefabCatalog", {0.f, 245.f}, true);
		for (int i = 0; i < static_cast<int>(prefabs.size()); ++i)
		{
			auto lowered = prefabs[i].name;
			std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (s_category && prefabs[i].category != s_categories[s_category])
			{
				continue;
			}
			if (!query.empty() && lowered.find(query) == std::string::npos)
			{
				continue;
			}
			ImGui::PushID(prefabs[i].name.c_str());
			if (ImGui::Selectable(std::format("{}  <{}>", prefabs[i].name, prefabs[i].category).c_str(), s_selected == i))
			{
				s_selected = i;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		if (s_selected >= 0 && s_selected < static_cast<int>(prefabs.size()))
		{
			if (!s_position_initialized)
			{
				if (auto player = unity::get_local_player())
				{
					s_position             = unity::get_position(player) + unity::get_forward(player) * 4.f;
					s_rotation             = Vector3{};
					s_position_initialized = true;
				}
			}
			ImGui::Text("Selected  %s", prefabs[s_selected].name.c_str());
			if (prefabs[s_selected].category == "Building" && ImGui::Button("Find Matching Build Tool", {-1.f, 0.f}))
			{
				find_matching_build_tool(prefabs[s_selected].name);
			}
			astra::slider("Spawn count", &s_count, 1, 100);
			ImGui::InputFloat3("Position XYZ", &s_position.x, "%.2f");
			ImGui::InputFloat3("Rotation XYZ", &s_rotation.x, "%.1f deg");
			if (ImGui::Button("Spawn Prefab", {-1.f, 0.f}))
			{
				spawn_prefab(prefabs[s_selected].name, s_count, s_position, s_rotation);
			}
		}
		ImGui::Separator();
		{
			std::lock_guard lock(s_prefab_mutex);
			ImGui::Text("Session history  /  %zu spawned", s_history.size());
			if (!s_history.empty())
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Undo last"))
				{
					undo_last();
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s", s_history.back().name.c_str());
			}
		}
		canvas::end_panel();
	}
}
