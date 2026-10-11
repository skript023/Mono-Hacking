#include "task_mgr.hpp"
#include "logger/exception_handler.hpp"

namespace big
{
	void task_mgr::add_impl(task<void> t)
	{
		std::lock_guard lock(m_mutex);
		m_tasks.push_back(std::move(t));
	}

	void task_mgr::add_impl(task<void> (*func)())
	{
		if (func)
		{
			std::lock_guard lock(m_mutex);
			m_tasks.push_back(func());
		}
	}

	void task_mgr::clear_impl()
	{
		std::lock_guard lock(m_mutex);
		m_tasks.clear();
	}

	void task_mgr::tick_impl()
	{
		std::lock_guard lock(m_mutex);

		for (auto it = m_tasks.begin(); it != m_tasks.end();)
		{
			auto& handle = it->m_handle;

			// Bersihkan jika handle kosong atau sudah selesai
			if (!handle || handle.done())
			{
				it = m_tasks.erase(it);
				continue;
			}

			auto* leaf = handle.promise().get_leaf();
			bool should_run = true;

			// Periksa status jeda/waktu tunggu di leaf Task
			if (leaf->m_wake_time.has_value())
			{
				if (std::chrono::high_resolution_clock::now() < leaf->m_wake_time.value())
				{
					should_run = false; // Belum waktunya jalan
				}
				else
				{
					leaf->m_wake_time = std::nullopt; // Reset wake time
				}
			}

			// Resume eksekusi coroutine pada leaf aktif
			if (should_run)
			{
				TRY_CLAUSE
				{
					leaf->get_handle().resume();
				}
				EXCEPT_CLAUSE
			}

			// Cek lagi setelah dijalankan, barangkali root task baru saja selesai
			if (handle && handle.done())
			{
				it = m_tasks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
