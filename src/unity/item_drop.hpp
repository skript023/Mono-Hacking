#pragma once
#include "mono/mono.hpp"
#include "item_data.hpp"

namespace big
{
    class item_drop
    {
        MonoObject* obj;
    public:
        item_drop(MonoObject* o);
        ~item_drop() noexcept;
        MonoObject* get_object();
        static std::vector<item_drop> get_drops();
        void set_stack(int stack);
		void set_quality(int quality);

		operator bool() const { return obj != nullptr; }
    };
}