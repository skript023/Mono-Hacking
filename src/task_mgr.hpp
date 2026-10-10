#pragma once
#include "common.hpp"
#include "task.hpp"
#include <vector>
#include <mutex>

namespace big
{
	class task_mgr
	{
		static task_mgr& get_instance()
		{
			static task_mgr instance;
			return instance;
		}

		// Hanya root task (task<void>) yang dimasukkan ke manager
		void add_impl(task<void> t);
		void add_impl(task<void> (*func)());
		void clear_impl();
		void tick_impl();

	public:
		task_mgr() = default;
		~task_mgr() = default;

		static void add(task<void> t)
		{
			get_instance().add_impl(std::move(t));
		}

		static void add(task<void> (*func)())
		{
			get_instance().add_impl(func);
		}

		static void clear()
		{
			get_instance().clear_impl();
		}

		static void tick()
		{
			get_instance().tick_impl();
		}

	private:
		std::recursive_mutex m_mutex;
		std::vector<task<void>> m_tasks;
	};
}
