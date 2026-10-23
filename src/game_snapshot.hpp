#pragma once
#include "task_mgr.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

namespace big
{
	template<typename T>
	class game_snapshot
	{
	public:
		explicit game_snapshot(std::function<T()> read) :
		    m_read(std::move(read))
		{
		}

		T get(std::chrono::milliseconds max_age)
		{
			const auto now = std::chrono::steady_clock::now();
			if (now - m_requested >= max_age && !m_refreshing.exchange(true))
			{
				m_requested = now;
				async_run([this] {
					auto value = m_read();
					{
						std::lock_guard lock(m_mutex);
						m_value = std::move(value);
					}
					m_refreshing = false;
				});
			}
			std::lock_guard lock(m_mutex);
			return m_value;
		}

	private:
		std::function<T()> m_read;
		std::mutex m_mutex;
		T m_value{};
		std::atomic_bool m_refreshing = false;
		std::chrono::steady_clock::time_point m_requested{};
	};
}
