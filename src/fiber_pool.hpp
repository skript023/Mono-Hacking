#pragma once

// Compatibility header for external dependencies (e.g. astra-ui).
// Win32 fibers have been fully migrated to C++20 coroutines (task.hpp & task_mgr.hpp).
#include "task.hpp"
#include "task_mgr.hpp"

namespace big
{
	class fiber_pool
	{
	public:
		template<typename F>
		void queue_job(F&& func)
		{
			async_run(std::forward<F>(func));
		}
	};

	inline fiber_pool* g_fiber_pool = reinterpret_cast<fiber_pool*>(0x1);
}
