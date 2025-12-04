#pragma once
#include "quickjspp.hpp"

namespace big
{
	class javascript_manager
	{
		static javascript_manager& get()
		{
			static javascript_manager i{};

			return i;
		}

	public:

	};
}