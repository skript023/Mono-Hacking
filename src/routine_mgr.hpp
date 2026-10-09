#pragma once
#include "common.hpp"
#include "routine.hpp"
#include <vector>
#include <mutex>

namespace big
{
	class routine_mgr
	{
	public:
		routine_mgr() = default;
		~routine_mgr() = default;

		void add(routine r);
		void clear();
		void tick();

	private:
		std::recursive_mutex m_mutex;
		std::vector<routine> m_routines;
	};

	inline routine_mgr g_routine_mgr;
}
