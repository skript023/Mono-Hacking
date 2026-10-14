#pragma once

namespace big
{
	class world_time
	{
	public:
		static bool set_time_scale(float value);
		static bool reset_time_scale();
		static bool is_time_paused();
		static bool set_day_fraction(float fraction);
		static bool next_morning();
		static bool save_world();
		static void shutdown();
		static bool cleanup_pending();
	};
}
