#pragma once
#include <imgui.h>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace big
{
	class item_icons
	{
	public:
		static void invalidate()
		{
			instance().invalidate_impl();
		}
		static void begin_frame()
		{
			instance().begin_frame_impl();
		}
		static ImTextureID get(const std::string& prefab)
		{
			return instance().get_impl(prefab);
		}

	private:
		item_icons() = default;
		item_icons(const item_icons&) = delete;
		item_icons& operator=(const item_icons&) = delete;
		static item_icons& instance()
		{
			static item_icons value;
			return value;
		}

		void invalidate_impl();
		void begin_frame_impl();
		ImTextureID get_impl(const std::string& prefab);
		void load_next_impl();
		std::vector<unsigned char> read_icon_impl(const std::string& prefab_name);

		using clock = std::chrono::steady_clock;
		static constexpr size_t cache_limit = 128;
		static constexpr int icon_size = 32;
		enum class state
		{
			queued,
			loading,
			ready,
			failed
		};
		struct entry
		{
			state status = state::queued;
			std::vector<unsigned char> pixels;
			ImTextureID texture = 0;
			int last_frame = 0;
			clock::time_point retry_at{};
			uint64_t ticket = 0;
		};
		std::mutex cache_mutex;
		std::unordered_map<std::string, entry> cache;
		uint64_t revision = 0;
		uint64_t next_ticket = 0;
		uint64_t renderer_generation = 0;
		bool reset_requested = false;
		bool job_pending = false;
		int upload_frame = -1;
	};
}
