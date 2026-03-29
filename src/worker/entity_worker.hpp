#pragma once
#include "class/double_buffer.hpp"

namespace big
{
	struct esp_data
    {
        Vector3 location;
        Vector3 screen;
        float distance;

        std::string display_classname;
        std::string display_text;
    };

	inline DoubleBuffer<esp_data> g_esp_data;

	class entity_worker
	{
	public:
		static void run();
	};
}