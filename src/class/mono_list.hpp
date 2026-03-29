#pragma once

template <typename T>
struct mono_list
{
    void* klass;
    void* monitor;
    T* items;
    int size;
    int capacity;
};