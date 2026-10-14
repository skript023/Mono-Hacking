#pragma once

namespace big::camera_tools
{
	bool freecam_active();
	bool toggle_freecam();
	bool apply_settings();
	void reset();
	void shutdown();
	bool cleanup_pending();
}
