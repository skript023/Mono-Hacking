#pragma once
#include "mono/mono.hpp"

namespace big
{
    class shared_data
    {
        MonoObject* m_shared;
    public:
        shared_data(MonoObject*);
        ~shared_data() noexcept;
		float get_max_durability();
		float get_durability_per_level();
	};
}