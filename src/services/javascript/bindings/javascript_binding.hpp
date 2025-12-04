#pragma once
#include "../quickjspp.hpp"

namespace big
{
	using namespace qjs;

	class javacript_binding
	{
		static javacript_binding& get()
		{
			static javacript_binding i{};

			return i;
		}

	public:
		static void bind(Context& ctx);

	};
}