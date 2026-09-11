#pragma once
#include "mono/mono.hpp"

namespace big
{
    class food
    {
        MonoObject* m_food;
    public:
        food(MonoObject* food);
        ~food() noexcept;
        MonoObject* get_object() const { return m_food; }
        std::string get_name();
        std::string get_shared_name();
        float get_time();
        float get_health();
        float get_stamina();
        float get_eitr();
        void set_time(float time);
        void set_health(float h);
        void set_stamina(float s);
        void set_eitr(float e);
        bool can_eat_again();
    };
}
