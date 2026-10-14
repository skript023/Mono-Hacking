#include "crafting_tools.hpp"
#include "commands/bool_command.hpp"
#include "commands/commands.hpp"
#include "localization.hpp"
#include "item_spawner.hpp"
#include "menu_settings.hpp"
#include "task_mgr.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <cctype>
#include <imgui.h>

namespace big
{
	bool crafting_tools::requested_impl() const
	{
		auto option = commands::get_command<bool_command>("open_all_recepies"_hash);
		return g_running && (g_settings.self.free_crafting || (option && option->get_state()));
	}
	bool crafting_tools::active_impl(MonoObject* player) const
	{
		return m_active && requested_impl() && (!player || player == m_player);
	}
	MonoObject* crafting_tools::ingredient_impl(MonoObject* recipe, int* amount, int* extra) const
	{
		auto entry = m_ingredients.find(recipe);
		if (entry == m_ingredients.end())
		{
			return nullptr;
		}
		if (amount)
		{
			*amount = 0;
		}
		if (extra)
		{
			*extra = entry->second.extra;
		}
		return entry->second.item;
	}
	MonoObject* crafting_tools::field_impl(MonoObject* object, const char* name)
	{
		if (!object)
		{
			return nullptr;
		}
		auto field = mono::get_field(mono::object_get_class(object), name);
		if (!field)
		{
			throw std::runtime_error(std::format("Missing crafting field: {}", name));
		}
		MonoObject* result = nullptr;
		mono::get_field_value(object, field, &result);
		return result;
	}
	MonoObject* crafting_tools::call_impl(MonoMethod* method, MonoObject* object, void** args)
	{
		if (!method)
		{
			throw std::runtime_error("Missing crafting method");
		}
		MonoObject* exception = nullptr;
		auto result           = mono::invoke_method(method, object, args, &exception);
		if (exception)
		{
			throw std::runtime_error("Crafting operation raised a managed exception");
		}
		return result;
	}
	void crafting_tools::retain_impl(MonoObject* object)
	{
		if (!object)
		{
			return;
		}
		auto root = mono::retain(object);
		if (!root)
		{
			throw std::runtime_error("Could not retain crafting object");
		}
		m_roots.push_back(root);
	}
	void crafting_tools::reset_impl()
	{
		m_active = false;
		m_player = nullptr;
		m_ingredients.clear();
		for (auto root : m_roots)
		{
			mono::release(root);
		}
		m_roots.clear();
	}
	void crafting_tools::cache_recipes_impl(MonoObject* database)
	{
		auto recipes = field_impl(database, "m_recipes");
		if (!recipes)
		{
			throw std::runtime_error("Recipe database not ready");
		}
		for (auto recipe : unity::list_to_vector(recipes))
		{
			if (!recipe)
			{
				continue;
			}
			auto resources = reinterpret_cast<MonoArray*>(field_impl(recipe, "m_resources"));
			if (!resources)
			{
				continue;
			}
			for (int index = 0; index < mono::array_length(resources); ++index)
			{
				auto requirement = *static_cast<MonoObject**>(mono::array_with_size(resources, sizeof(MonoObject*), index));
				if (!requirement)
				{
					continue;
				}
				auto item = field_impl(field_impl(requirement, "m_resItem"), "m_itemData");
				if (!item)
				{
					continue;
				}
				int extra = 0;
				auto extra_field = mono::get_field(mono::object_get_class(requirement), "m_extraAmountOnlyOneIngredient");
				if (extra_field)
				{
					mono::get_field_value(requirement, extra_field, &extra);
				}
				const bool inserted = m_ingredients.emplace(recipe, ingredient_entry{item, extra}).second;
				if (inserted)
				{
					retain_impl(recipe);
					retain_impl(item);
				}
				break;
			}
		}
	}
	void crafting_tools::refresh_ui_impl(bool force)
	{
		auto gui = call_impl(mono::get_method("InventoryGui", "get_instance", 0, "assembly_valheim"), nullptr);
		if (!gui)
		{
			return;
		}
		if (force)
		{
			bool focus   = false;
			void* args[] = {&focus};
			call_impl(mono::get_method("InventoryGui", "UpdateCraftingPanel", 1, "assembly_valheim"), gui, args);
		}
		if (m_active)
		{
			auto button = field_impl(gui, "m_tabUpgrade");
			auto go = button ? call_impl(mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine"), button) : nullptr;
			if (go)
			{
				bool enabled = true;
				void* args[] = {&enabled};
				call_impl(mono::get_method("GameObject", "SetActive", 1, "UnityEngine.CoreModule", "UnityEngine"), go, args);
			}
		}
	}
	void crafting_tools::update_impl()
	{
		apply_crafting_speed_impl();
		if (std::chrono::steady_clock::now() < m_retry_after)
		{
			return;
		}
		const bool requested  = requested_impl();
		const bool cache_only = g_settings.self.free_material_costs;
		if (!requested && !cache_only && !m_active && !m_player)
		{
			return;
		}
		try
		{
			auto player = unity::get_local_player();
			if ((!requested && !cache_only) || !player)
			{
				bool refresh = m_active && player;
				reset_impl();
				if (refresh)
				{
					refresh_ui_impl(true);
				}
				return;
			}
			bool fresh = player != m_player || (requested && !m_active);
			if (fresh)
			{
				reset_impl();
				auto db = unity::get_object_db();
				if (!db)
				{
					return;
				}
				cache_recipes_impl(db);
				if (requested)
				{
					retain_impl(player);
				}
				m_player         = player;
				m_active         = requested;
				m_next_discovery = {};
			}
			if (!requested)
			{
				return;
			}
			auto now = std::chrono::steady_clock::now();
			if (now >= m_next_discovery)
			{
				m_next_discovery = now + std::chrono::seconds(2);
				call_impl(mono::get_method("Player", "UpdateKnownRecipesList", 0, "assembly_valheim"), player);
			}
			refresh_ui_impl(fresh);
		}
		catch (const std::exception& error)
		{
			reset_impl();
			m_retry_after = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			LOG(WARNING) << "[Crafting] " << error.what();
		}
	}

	void crafting_tools::apply_crafting_speed_impl()
	{
		static auto get_gui = mono::get_method("InventoryGui", "get_instance", 0, "assembly_valheim");
		if (!get_gui)
		{
			return;
		}
		auto gui = mono::invoke_method(get_gui, nullptr);
		if (!gui)
		{
			return;
		}
		if (gui != m_craft_gui)
		{
			m_craft_gui                 = gui;
			m_base_craft_duration       = mono::get_field_value<"InventoryGui", "m_craftDuration", float>(gui);
			m_base_multi_craft_duration = mono::get_field_value<"InventoryGui", "m_multiCraftDuration", float>(gui);
			if (m_base_craft_duration <= 0.f)
			{
				m_base_craft_duration = 2.f;
			}
			if (m_base_multi_craft_duration <= 0.f)
			{
				m_base_multi_craft_duration = 6.f;
			}
		}

		const float multiplier           = std::clamp(g_settings.self.crafting_speed_multiplier, 0.1f, 20.f);
		const float craft_duration       = std::max(0.05f, m_base_craft_duration / multiplier);
		const float multi_craft_duration = std::max(0.05f, m_base_multi_craft_duration / multiplier);
		mono::set_field_value<"InventoryGui", "m_craftDuration">(gui, craft_duration);
		mono::set_field_value<"InventoryGui", "m_multiCraftDuration">(gui, multi_craft_duration);
	}
	void crafting_tools::queue_update_impl()
	{
		if (m_pending)
		{
			return;
		}
		m_pending = true;
		async_run([this] {
			m_pending = false;
			if (g_running)
			{
				update_impl();
			}
			else
			{
				reset_impl();
			}
		});
	}
	void crafting_tools::shutdown_impl()
	{
		m_cleanup_pending.store(true);
		async_run([this] {
			reset_impl();
			if (m_craft_gui)
			{
				mono::set_field_value<"InventoryGui", "m_craftDuration">(m_craft_gui, m_base_craft_duration);
				mono::set_field_value<"InventoryGui", "m_multiCraftDuration">(m_craft_gui, m_base_multi_craft_duration);
				m_craft_gui = nullptr;
			}
			m_cleanup_pending.store(false);
		});
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (m_cleanup_pending.load() && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void crafting_tools::toggle_recipe_browser_impl()
	{
		m_browser_open = !m_browser_open;
		if (m_browser_open)
		{
			refresh_recipe_browser_impl();
		}
	}

	void crafting_tools::refresh_recipe_browser_impl()
	{
		if (m_browser_refreshing.exchange(true))
		{
			return;
		}

		async_run([this] {
			try
			{
				scan_recipe_browser_impl();
			}
			catch (const std::exception& error)
			{
				LOG(WARNING) << "[Recipe Browser] " << error.what();
			}
			m_browser_refreshing = false;
		});
	}

	void crafting_tools::scan_recipe_browser_impl()
	{
		auto database = unity::get_object_db();
		if (!database)
		{
			return;
		}

		auto recipes_object = field_impl(database, "m_recipes");
		std::vector<recipe_row> recipes;
		for (auto* recipe : unity::list_to_vector(recipes_object))
		{
			if (!recipe)
			{
				continue;
			}
			try
			{
				auto output_drop   = field_impl(recipe, "m_item");
				auto output_data   = output_drop ? field_impl(output_drop, "m_itemData") : nullptr;
				auto output_shared = output_data ? field_impl(output_data, "m_shared") : nullptr;
				if (!output_data || !output_shared)
				{
					continue;
				}

				auto name_string = reinterpret_cast<MonoString*>(field_impl(output_shared, "m_name"));
				auto drop_prefab = field_impl(output_data, "m_dropPrefab");
				if (!name_string || !drop_prefab)
				{
					continue;
				}

				recipe_row row;
				row.name          = localization::get_instance().localize(name_string);
				row.output_prefab = unity::get_name(drop_prefab);
				mono::get_field_value(recipe, mono::get_field(mono::object_get_class(recipe), "m_amount"), &row.output_amount);
				if (row.name.empty() || row.output_prefab.empty())
				{
					continue;
				}

				auto resources = reinterpret_cast<MonoArray*>(field_impl(recipe, "m_resources"));
				if (resources)
				{
					for (int index = 0; index < mono::array_length(resources); ++index)
					{
						auto requirement = *static_cast<MonoObject**>(mono::array_with_size(resources, sizeof(MonoObject*), index));
						if (!requirement)
						{
							continue;
						}
						auto resource_drop   = field_impl(requirement, "m_resItem");
						auto resource_data   = resource_drop ? field_impl(resource_drop, "m_itemData") : nullptr;
						auto resource_shared = resource_data ? field_impl(resource_data, "m_shared") : nullptr;
						auto resource_name = resource_shared ? reinterpret_cast<MonoString*>(field_impl(resource_shared, "m_name")) : nullptr;
						if (!resource_name)
						{
							continue;
						}

						int amount = 0;
						mono::get_field_value(requirement, mono::get_field(mono::object_get_class(requirement), "m_amount"), &amount);
						if (!row.requirements.empty())
						{
							row.requirements += "  |  ";
						}
						row.requirements += std::format("{} x{}", localization::get_instance().localize(resource_name), amount);
					}
				}
				recipes.push_back(std::move(row));
			}
			catch (const std::exception&)
			{
				continue;
			}
		}

		std::sort(recipes.begin(), recipes.end(), [](const recipe_row& left, const recipe_row& right) {
			return left.name < right.name;
		});
		std::lock_guard lock(m_browser_mutex);
		m_browser_recipes = std::move(recipes);
	}

	void crafting_tools::draw_recipe_browser_impl()
	{
		if (!m_browser_open)
		{
			return;
		}

		ImGui::SetNextWindowSize({620.f, 560.f}, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Runtime Recipe Browser", &m_browser_open, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		ImGui::SetNextItemWidth(-132.f);
		ImGui::InputTextWithHint("##RecipeSearch", "Search recipes and outputs", m_browser_search, sizeof(m_browser_search));
		ImGui::SameLine();
		if (ImGui::Button(m_browser_refreshing ? "Scanning..." : "Refresh", {120.f, 0.f}) && !m_browser_refreshing)
		{
			refresh_recipe_browser_impl();
		}

		std::vector<recipe_row> snapshot;
		{
			std::lock_guard lock(m_browser_mutex);
			snapshot = m_browser_recipes;
		}
		ImGui::TextDisabled("Runtime recipes: %zu  |  Output opens in Item Spawner", snapshot.size());
		ImGui::Separator();
		ImGui::BeginChild("##RecipeResults", {0.f, 0.f}, true);
		std::string filter = m_browser_search;
		std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		for (const auto& recipe : snapshot)
		{
			std::string searchable = recipe.name + " " + recipe.output_prefab + " " + recipe.requirements;
			std::transform(searchable.begin(), searchable.end(), searchable.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (!filter.empty() && searchable.find(filter) == std::string::npos)
			{
				continue;
			}
			ImGui::PushID(recipe.output_prefab.c_str());
			ImGui::TextUnformatted(recipe.name.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("%s x%d", recipe.output_prefab.c_str(), recipe.output_amount);
			if (!recipe.requirements.empty())
			{
				ImGui::TextWrapped("Materials: %s", recipe.requirements.c_str());
			}
			else
			{
				ImGui::TextDisabled("No listed materials");
			}
			if (ImGui::SmallButton("Open Output in Item Spawner"))
			{
				item_spawner::focus_item(recipe.output_prefab);
			}
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::EndChild();
		ImGui::End();
	}
}
