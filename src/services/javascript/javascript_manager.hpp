#pragma once
#include "quickjspp.hpp"

namespace big
{
	class javascript_manager
	{
		qjs::Runtime m_runtime;
		qjs::Context m_context;

		javascript_manager() :
		    m_context(m_runtime)
		{
		} // Context depends on Runtime

		// disable copy
		javascript_manager(const javascript_manager&) = delete;
		javascript_manager& operator=(const javascript_manager&) = delete;

		static javascript_manager& get()
		{
			static javascript_manager instance;

			return instance;
		}

		void init_impl();
	public:
		static void init()
		{
			get().init_impl();
		}
	};
}