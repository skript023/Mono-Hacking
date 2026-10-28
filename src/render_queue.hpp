#pragma once
#include <functional>
#include <mutex>
#include <vector>

namespace big
{
	class render_queue
	{
	public:
		static void post(std::function<void()> job)
		{
			auto& self = instance();
			std::lock_guard lock(self.m_mutex);
			self.m_jobs.push_back(std::move(job));
		}

		static void run()
		{
			auto& self = instance();
			std::vector<std::function<void()>> jobs;
			{
				std::lock_guard lock(self.m_mutex);
				jobs.swap(self.m_jobs);
			}
			for (auto& job : jobs)
			{
				job();
			}
		}

	private:
		static render_queue& instance()
		{
			static render_queue value;
			return value;
		}

		std::mutex m_mutex;
		std::vector<std::function<void()>> m_jobs;
	};
}
