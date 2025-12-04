#pragma once

namespace big
{
	class javacript_binding
	{
		static javacript_binding& get()
		{
			static javacript_binding i{};

			return i;
		}

	public:
		static void init();

	};
}