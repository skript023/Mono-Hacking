#pragma once
#include "mono/mono.hpp"
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace big
{
	class crafting_tools
	{
	public:
		static void update()
		{
			instance().queue_update_impl();
		}
		static bool active(MonoObject* player = nullptr)
		{
			return instance().active_impl(player);
		}
		static MonoObject* ingredient(MonoObject* recipe, int* amount, int* extra)
		{
			return instance().ingredient_impl(recipe, amount, extra);
		}
		static void shutdown()
		{
			instance().shutdown_impl();
		}
		static bool cleanup_pending()
		{
			return instance().cleanup_pending_impl();
		}

	private:
		crafting_tools() = default;
		crafting_tools(const crafting_tools&) = delete;
		crafting_tools& operator=(const crafting_tools&) = delete;
		static crafting_tools& instance()
		{
			static crafting_tools value;
			return value;
		}
		void queue_update_impl();
		void update_impl();
		void reset_impl();
		void shutdown_impl();
		bool cleanup_pending_impl() const
		{
			return m_cleanup_pending.load();
		}
		bool active_impl(MonoObject* player) const;
		bool requested_impl() const;
		MonoObject* ingredient_impl(MonoObject* recipe, int* amount, int* extra) const;
		MonoObject* field_impl(MonoObject* object, const char* name);
		MonoObject* call_impl(MonoMethod* method, MonoObject* object, void** args = nullptr);
		void retain_impl(MonoObject* object);
		void cache_recipes_impl(MonoObject* database);
		void refresh_ui_impl(bool force);
		struct ingredient_entry
		{
			MonoObject* item;
			int extra;
		};
		std::unordered_map<MonoObject*, ingredient_entry> m_ingredients;
		std::vector<uintptr_t> m_roots;
		MonoObject* m_player = nullptr;
		bool m_active = false;
		bool m_pending = false;
		std::atomic<bool> m_cleanup_pending{false};
		std::chrono::steady_clock::time_point m_next_discovery{};
		std::chrono::steady_clock::time_point m_retry_after{};
	};
}
