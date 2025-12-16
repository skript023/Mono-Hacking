#include "store_binding.hpp"
#include "ui/store_mgr.hpp"

namespace js::storage
{
	using namespace big;

	static double js_store_get_float(std::string const key)
	{
		float result = store::get_float(key);

		return result;
	}

	static void js_store_set_float(std::string const& key, double value)
	{
		store::set_float(key, (float)value);
	}

	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;

		auto global = context.global();
		auto store = context.newObject();

		store.add<&store::get_bool>("get_bool");
		store.add<&store::set_bool>("set_bool");
		store.add<&store::set_int>("set_int");
		store.add<&store::get_int>("get_int");
		store.add<&js_store_get_float>("get_float");
		store.add<&js_store_set_float>("set_float");

		global["store"] = store;
	}
}