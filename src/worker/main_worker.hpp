#pragma once
#include "task.hpp"

namespace big
{
	class main_worker
	{
	public:
		static void run();
		static void slow_run();
		static task<void> run_routine();
	};
}
