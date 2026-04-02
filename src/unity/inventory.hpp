#pragma once
#include "mono/mono.hpp"

namespace big
{
	class inventory
	{
        MonoObject* m_inventory;
	public:
        inventory(MonoObject* obj);
        ~inventory() noexcept;

        void set_height(int height);
        void set_width(int width);
        int get_height();
        int get_width();
	};
}