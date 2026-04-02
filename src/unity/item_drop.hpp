#pragma once
#include "mono/mono.hpp"
#include "item_data.hpp"
#include "localization.hpp"

namespace big
{
    class item_drop
    {
        MonoObject* obj;
        item_data m_data;
        localization m_localization;
    public:
        item_drop(MonoObject* o);
        ~item_drop() noexcept;
        MonoObject* get_object() const;
        static std::vector<item_drop> get_drops();
        void set_stack(int stack);
		void set_quality(int quality);
		std::optional<Vector3> get_position();
		std::optional<Vector3> get_bounds_top();
		std::string get_hover_name();
		std::string get_hover_text();

		operator bool() const { return obj != nullptr; }
    };
}