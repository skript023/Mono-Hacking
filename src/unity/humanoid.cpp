#include "character.hpp"
#include "humanoid.hpp"

namespace big
{
    inventory humanoid::get_inventory()
    {
        auto method = mono::get_method("Humanoid", "GetInventory", 0, "assembly_valheim");

        if (!method)
        {
            LOG(FATAL) << "Method not found";
        }

        inventory ret = mono::invoke_method(method, m_character);

        return ret;
    }
}