#pragma once
#include "mono/mono.hpp"

namespace big
{
    class item_data
    {
        MonoObject* obj;
    public:
        item_data(MonoObject* o);
        ~item_data() noexcept;
		MonoObject* get_object();
	};
}