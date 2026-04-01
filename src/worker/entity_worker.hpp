#pragma once
#include "class/enums.hpp"
#include "class/double_buffer.hpp"

namespace big
{
	struct esp_data
    {
        Vector3 location;
        Vector3 screen;
        float distance;

        std::string name;
		float health;
		float max_health;
		Vector3 top;
		Vector3 bottom;

		EEntityType type;
    };

	inline DoubleBuffer<esp_data> g_esp_data;

	class entity_worker
	{
	public:
		static void run();
	};
}