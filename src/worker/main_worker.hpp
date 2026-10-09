#pragma once
#include "routine.hpp"

namespace big
{
	class main_worker
	{
	public:
		static void run();
		static void slow_run();
		static routine run_routine();
	};
}
