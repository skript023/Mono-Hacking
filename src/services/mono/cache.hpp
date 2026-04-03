#pragma once
#include "utility/joaat.hpp"

namespace big
{
	template <size_t N>
	struct const_str
	{
		char value[N];

		constexpr const_str(const char (&str)[N])
		{
			for (size_t i = 0; i < N; i++)
				value[i] = str[i];
		}
	};
}