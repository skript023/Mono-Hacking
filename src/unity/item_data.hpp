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

        void set_durability(float durability);
        void set_quality(int quality);
        void set_stack(int stack);
        void set_variant(int variant);

		void set_grid_pos(iVector2 pos);

		operator bool() const { return obj != nullptr; }
	};
}