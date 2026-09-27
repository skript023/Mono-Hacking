#pragma once
#include "mono/mono.hpp"
#include "class/vector.hpp"
#include <array>
#include <atomic>
#include <vector>

namespace big::unity
{
	class hud_manager
	{
	public:
		static void shutdown()
		{
			instance().shutdown_impl();
		}
		static bool cleanup_pending()
		{
			return instance().cleanup_pending_impl();
		}
		static void update()
		{
			instance().queue_update_impl();
		}
		static void reset()
		{
			instance().queue_reset_impl();
		}

	private:
		hud_manager() = default;
		hud_manager(const hud_manager&) = delete;
		hud_manager& operator=(const hud_manager&) = delete;
		static hud_manager& instance()
		{
			static hud_manager value;
			return value;
		}
		void queue_update_impl();
		void shutdown_impl();
		bool cleanup_pending_impl() const;
		void queue_reset_impl();
		void update_impl();
		void reset_impl();
		void restore_impl();
		void resize_impl(MonoObject* hud, int target);
		MonoMethod* method_impl(const char* type, const char* name, int count);
		MonoObject* call_impl(MonoMethod* method, MonoObject* object = nullptr, void** args = nullptr);
		MonoObject* element_impl(MonoArray* array, int index);
		MonoObject* clone_impl(MonoObject* source, const Vector3& offset);
		uintptr_t retain_impl(MonoObject* object);
		bool m_pending = false;
		std::atomic<bool> m_cleanup_pending{false};
		bool m_failed = false;
		int m_target = 3;
		uintptr_t m_hud = 0;
		std::array<MonoClassField*, 3> m_fields{};
		std::array<uintptr_t, 3> m_originals{};
		std::vector<uintptr_t> m_roots;
		std::vector<uintptr_t> m_clones;
	};
}
