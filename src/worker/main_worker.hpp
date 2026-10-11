#pragma once
#include "task.hpp"

namespace big
{
	class main_worker
	{
	public:
		static task<void> run();
		static task<void> slow_run();
	};
}
