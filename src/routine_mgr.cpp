#include "routine_mgr.hpp"

namespace big
{
	void routine_mgr::add(routine r)
	{
		std::lock_guard lock(m_mutex);
		m_routines.push_back(r);
	}

	void routine_mgr::clear()
	{
		std::lock_guard lock(m_mutex);
		for (auto& r : m_routines)
		{
			if (r.m_handle)
			{
				r.m_handle.destroy();
			}
		}
		m_routines.clear();
	}

	void routine_mgr::tick()
	{
		std::lock_guard lock(m_mutex);

		for (auto it = m_routines.begin(); it != m_routines.end(); )
		{
			auto& handle = it->m_handle;

			// Bersihkan jika handle kosong atau sudah selesai
			if (!handle || handle.done())
			{
				if (handle)
				{
					handle.destroy();
				}
				it = m_routines.erase(it);
				continue;
			}

			auto& promise = handle.promise();
			bool should_run = true;

			// Periksa apakah routine sedang dalam status jeda/waktu tunggu
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

			// Resume coroutine
			if (should_run)
			{
				handle.resume();
			}

			// Cek lagi setelah dijalankan, barangkali langsung selesai (seluruh line kode sudah habis)
			if (handle && handle.done())
			{
				handle.destroy();
				it = m_routines.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
