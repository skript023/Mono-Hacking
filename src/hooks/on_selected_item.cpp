#include "hooking.hpp"
#include <script_mgr.hpp>

namespace big
{
	void hooks::on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod)
	{
        if (item_data_obj != nullptr)
        {
            // item_data_obj adalah MonoObject* dari ItemDrop.ItemData
            LOG(INFO) << "Item diklik! Alamat ItemDrop.ItemData: " << item_data_obj;

            // 1. Dapatkan Class ItemDrop.ItemData
            MonoClass* item_data_class = mono::get_class("ItemDrop/ItemData", "assembly_valheim");

            if (item_data_class != nullptr)
            {
                LOG(INFO) << "SUCCESS: Class ItemDrop+ItemData Ditemukan.";
                // 2. Akses Data Item (Misalnya, Durability)
                MonoClassField* durability = mono::get_field(item_data_class, "m_durability");
                MonoClassField* quality = mono::get_field(item_data_class, "m_quality");
                MonoClassField* stack = mono::get_field(item_data_class, "m_stack");
                MonoClassField* variant = mono::get_field(item_data_class, "m_variant");
                float current_durability;

                // Dapatkan nilai field pada instance item_data_obj
                mono::get_field_value(item_data_obj, durability, &current_durability);

                // Cetak data yang diakses
                LOG(INFO) << "Durability Item: " << current_durability;

                // Anda juga bisa mengakses m_shared (ItemDrop.ItemData.SharedData)
                // Ini akan membutuhkan satu langkah reflection lagi.
            }

            // Dapatkan posisi grid
            LOG(INFO) << "Posisi Grid: (" << pos.x << ", " << pos.y << ")";
        }

		return detour_base::get_original<on_selected_item>()(this_ptr, grid_ptr, item_data_obj, pos, mod);
	}
}