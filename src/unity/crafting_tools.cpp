#include "crafting_tools.hpp"
#include "commands/bool_command.hpp"
#include "commands/commands.hpp"
#include "fiber_pool.hpp"
#include "utility/unity.hpp"

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
			return nullptr;
		if (amount)
			*amount = 0;
		if (extra)
			*extra = entry->second.extra;
		return entry->second.item;
	}
	MonoObject* crafting_tools::field_impl(MonoObject* object, const char* name)
	{
		if (!object)
			return nullptr;
		auto field = mono::get_field(mono::object_get_class(object), name);
		if (!field)
			throw std::runtime_error(std::format("Missing crafting field: {}", name));
		MonoObject* result = nullptr;
		mono::get_field_value(object, field, &result);
		return result;
	}
	MonoObject* crafting_tools::call_impl(MonoMethod* method, MonoObject* object, void** args)
	{
		if (!method)
			throw std::runtime_error("Missing crafting method");
		MonoObject* exception = nullptr;
		auto result = mono::invoke_method(method, object, args, &exception);
		if (exception)
			throw std::runtime_error("Crafting operation raised a managed exception");
		return result;
	}
	void crafting_tools::retain_impl(MonoObject* object)
	{
		if (!object)
			return;
		auto root = mono::retain(object);
		if (!root)
			throw std::runtime_error("Could not retain crafting object");
		m_roots.push_back(root);
	}
	void crafting_tools::reset_impl()
	{
		m_active = false;
		m_player = nullptr;
		m_ingredients.clear();
		for (auto root : m_roots)
			mono::release(root);
		m_roots.clear();
	}
	void crafting_tools::cache_recipes_impl(MonoObject* database)
	{
		auto recipes = field_impl(database, "m_recipes");
		if (!recipes)
			throw std::runtime_error("Recipe database not ready");
		for (auto recipe : unity::list_to_vector(recipes))
		{
			if (!recipe)
				continue;
			auto resources = reinterpret_cast<MonoArray*>(field_impl(recipe, "m_resources"));
			if (!resources)
				continue;
			for (int index = 0; index < mono::array_length(resources); ++index)
			{
				auto requirement = *static_cast<MonoObject**>(mono::array_with_size(resources, sizeof(MonoObject*), index));
				auto item = field_impl(field_impl(requirement, "m_resItem"), "m_itemData");
				if (!item)
					continue;
				int extra = 0;
				mono::get_field_value(requirement, mono::get_field(mono::object_get_class(requirement), "m_extraAmountOnlyOneIngredient"), &extra);
				retain_impl(recipe);
				retain_impl(item);
				m_ingredients.emplace(recipe, ingredient_entry{item, extra});
				break;
			}
		}
	}
	void crafting_tools::refresh_ui_impl(bool force)
	{
		auto gui = call_impl(mono::get_method("InventoryGui", "get_instance", 0, "assembly_valheim"), nullptr);
		if (!gui)
			return;
		if (force)
		{
			bool focus = false;
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
		if (std::chrono::steady_clock::now() < m_retry_after)
			return;
		if (!requested_impl() && !m_active)
			return;
		try
		{
			auto player = unity::get_local_player();
			if (!requested_impl() || !player)
			{
				bool refresh = m_active && player;
				reset_impl();
				if (refresh)
					refresh_ui_impl(true);
				return;
			}
			bool fresh = player != m_player;
			if (fresh)
			{
				reset_impl();
				auto db = unity::get_object_db();
				if (!db)
					return;
				cache_recipes_impl(db);
				retain_impl(player);
				m_player = player;
				m_active = true;
				m_next_discovery = {};
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
	void crafting_tools::queue_update_impl()
	{
		if (m_pending || !g_fiber_pool)
			return;
		m_pending = true;
		g_fiber_pool->queue_job([this] {
			m_pending = false;
			if (g_running)
				update_impl();
			else
				reset_impl();
		});
	}
	void crafting_tools::shutdown_impl()
	{
		if (!g_fiber_pool)
			return;
		m_cleanup_pending.store(true);
		g_fiber_pool->queue_job([this] {
			reset_impl();
			m_cleanup_pending.store(false);
		});
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (m_cleanup_pending.load() && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
