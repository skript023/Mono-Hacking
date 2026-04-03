#pragma once
#include "core.hpp"

namespace big
{
    template<typename T>
    class mono_array_iterator
    {
    public:
        mono_array_iterator(MonoArray* arr, int index)
            : m_array(arr), m_index(index)
        {
        }

        T operator*() const
        {
            auto raw = *reinterpret_cast<MonoObject**>(mono_core::mono_array_addr_with_size(m_array, sizeof(MonoObject*), m_index));

            if constexpr (std::is_same_v<T, MonoObject*>)
                return raw;
            else
                return T(raw);
        }

        mono_array_iterator& operator++()
        {
            ++m_index;
            return *this;
        }

        bool operator!=(const mono_array_iterator& other) const
        {
            return m_index != other.m_index;
        }

    private:
        MonoArray* m_array{};
        int m_index{};
    };

    template<typename T>
    class mono_array_view
    {
    public:
        mono_array_view() = default;

        mono_array_view(MonoArray* arr, int size = -1)
            : m_array(arr), m_size(size) {}

        int size() const
        {
            if (!m_array) return 0;
            if (m_size >= 0) return m_size;

            return mono_core::mono_array_length(m_array);
        }

        T operator[](int index) const
        {
            auto raw = *reinterpret_cast<MonoObject**>(
                mono_core::mono_array_addr_with_size(m_array, sizeof(MonoObject*), index)
            );

            if constexpr (std::is_same_v<T, MonoObject*>)
                return raw;
            else
                return T(raw);
        }

        inline T at(int index) const
        {
            if (!m_array || index < 0 || index >= size())
                return T{};
            return (*this)[index];
        }

        auto begin() const
        {
            return mono_array_iterator<T>(m_array, 0);
        }

        auto end() const
        {
            return mono_array_iterator<T>(m_array, size());
        }
    private:
        MonoArray* m_array{};
        int m_size{-1};
    };

    template<typename T>
    class List
    {
    public:
        List() = default;

        explicit List(MonoObject* list)
        {
            reset(list);
        }

        void reset(MonoObject* list)
        {
            m_list = list;

            if (!m_list)
            {
                m_view = {};
                return;
            }

            auto klass = mono::object_get_class(m_list);

            auto items_field = mono::get_field(klass, "_items");
            auto size_field  = mono::get_field(klass, "_size");

            if (!items_field || !size_field)
            {
                m_view = {};
                return;
            }

            MonoObject* items{};
            int size{};

            mono::get_field_value(m_list, items_field, &items);
            mono::get_field_value(m_list, size_field, &size);

            if (!items || size <= 0)
            {
                m_view = {};
                return;
            }

            m_view = mono_array_view<T>(reinterpret_cast<MonoArray*>(items), size);
        }

        int size() const
        {
            return m_view.size();
        }

        bool empty() const
        {
            return size() == 0;
        }

        T operator[](int index) const
        {
            return m_view[index];
        }

        auto begin() const { return m_view.begin(); }
        auto end()   const { return m_view.end();   }

    private:
        MonoObject* m_list{};
        mono_array_view<T> m_view{};
    };
}