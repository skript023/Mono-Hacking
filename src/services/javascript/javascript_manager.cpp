#include "javascript_manager.hpp"
#include "bindings/javascript_binding.hpp"

namespace big
{
	void javascript_manager::init_impl()
	{
		javacript_binding::bind(m_context);
		try
		{
			m_context.eval(R"(
				import * as log from 'Logger';
				log.info("This is a test log from javascript");
				console.log('hello from js', 123, true));
			)", "<eval>", JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_TYPE_GLOBAL);
		}
		catch (qjs::exception&)
		{
			auto exc = m_context.getException();
			LOG(WARNING) << "[JS Error] " << (std::string)exc;
		}
	}
}