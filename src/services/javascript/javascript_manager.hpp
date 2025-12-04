#pragma once
#include "quickjspp.hpp"

namespace big
{
	class javascript_manager
	{
		qjs::Runtime m_runtime;
		qjs::Context m_context;

		static javascript_manager& get()
		{
			static javascript_manager i{};

			return i;
		}

		void init_impl();
	public:

	};
}