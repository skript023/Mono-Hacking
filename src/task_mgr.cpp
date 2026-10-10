#include "task_mgr.hpp"

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
		for (auto& t : m_tasks)
		{
			if (t.m_handle)
			{
				t.m_handle.destroy();
			}
		}
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
				if (handle)
				{
					handle.destroy();
				}
				it = m_tasks.erase(it);
				continue;
			}

			auto& promise = handle.promise();
			bool should_run = true;

			// Periksa status jeda/waktu tunggu di Root Task
			if (promise.m_wake_time.has_value())
			{
				if (std::chrono::high_resolution_clock::now() < promise.m_wake_time.value())
				{
					should_run = false; // Belum waktunya jalan
				}
				else
				{
					promise.m_wake_time = std::nullopt; // Reset wake time
				}
			}

			// Resume eksekusi coroutine
			if (should_run)
			{
				handle.resume();
			}

			// Cek lagi setelah dijalankan, barangkali baru saja menyentuh co_return
			if (handle && handle.done())
			{
				handle.destroy();
				it = m_tasks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
