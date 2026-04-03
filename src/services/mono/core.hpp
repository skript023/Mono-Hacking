#pragma once
#include "mono_functions.hpp"

namespace big
{
    class mono_core
    {
    public:
        using mono_array_addr_with_size_t = void* (*)(MonoArray*, int, uintptr_t);
        using mono_array_length_t         = int   (*)(MonoArray*);

        inline static mono_array_addr_with_size_t mono_array_addr_with_size = nullptr;
        inline static mono_array_length_t mono_array_length = nullptr;
    };
}