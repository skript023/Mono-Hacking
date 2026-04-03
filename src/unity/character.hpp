#pragma once
#include "mono/mono.hpp"

namespace big
{
	class character
	{
	protected:
		MonoObject* m_character{};
	private:
		struct cached_name_entry
		{
			std::string name;
			std::chrono::steady_clock::time_point last_update;
		};

		static inline std::unordered_map<MonoObject*, cached_name_entry> m_hover_name_cache;
	public:
		character(MonoObject* character);
		~character() noexcept;

		MonoObject* get_object();

		void set_health(float health);
		void set_max_health(float health);
		void set_tamed(bool tamed);
		float get_max_health();
		std::string get_hover_name();
		float get_health();
		bool is_dead();
		virtual bool is_player();
		Vector3 get_center_point();
		Vector3 get_forward();
		Vector3 get_head_point();
		Vector3 get_top_point();
		Vector3 get_velocity();
		Vector3 get_position();
		Vector4 get_rotation();
		Vector3 get_euler_angles();
		static mono_array_view<character> get_all_characters();
		static mono_array_view<character> get_all_scharacters();

		bool operator==(character const& c) const { return m_character == c.m_character; }
		operator bool() const { return m_character != nullptr; }
	};
}