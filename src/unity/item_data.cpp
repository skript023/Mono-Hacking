#include "item_data.hpp"

namespace big
{
	item_data::item_data(MonoObject* o): obj(o)
	{
	}
	item_data::~item_data() noexcept
	{
		obj = nullptr;
	}
	MonoObject* item_data::get_object()
	{
		return obj;
	}
}