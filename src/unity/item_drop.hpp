#pragma once
#include "mono/mono.hpp"
#include "item_data.hpp"
#include "localization.hpp"

namespace big
{
	class item_drop
	{
		item_drop() :
		    obj(nullptr),
		    m_localization(nullptr)
		{
		}
		static item_drop& instance()
		{
			static item_drop value;
			return value;
		}
		mono_array_view<item_drop> get_drops_impl();
		MonoObject* obj;
		localization m_localization;

	public:
		item_drop(MonoObject* o);
		~item_drop() noexcept;
		MonoObject* get_object() const;
		item_data get_data();
		static mono_array_view<item_drop> get_drops()
		{
			return instance().get_drops_impl();
		}
		void set_stack(int stack);
		void set_quality(int quality);
		std::optional<Vector3> get_position();
		std::optional<Vector3> get_bounds_top();
		bool can_pickup(bool autoPickupDelay);
		bool can_eat();
		std::string get_hover_name();
		std::string get_hover_text();
		void save();

		operator bool() const
		{
			return obj != nullptr;
		}
	};
}
