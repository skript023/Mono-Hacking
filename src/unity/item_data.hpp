#pragma once
#include "mono/mono.hpp"
#include "shared_data.hpp"

namespace big
{
    class item_data
    {
        MonoObject* obj;
    public:
        item_data(MonoObject* o);
        ~item_data() noexcept;
		MonoObject* get_object();

        shared_data get_shared_data();
        void set_durability(float durability);
		float get_durability();
		float get_max_durability();
		void set_quality(int quality);
		int get_quality();
		void set_stack(int stack);
		int get_stack();
		void set_variant(int variant);
		int get_variant();
		void set_grid_pos(iVector2 pos);
		iVector2 get_grid_pos();

		operator bool() const { return obj != nullptr; }
	};
}