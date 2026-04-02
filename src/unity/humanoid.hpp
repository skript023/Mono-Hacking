#pragma once
#include "mono/mono.hpp"
#include "character.hpp"
#include "inventory.hpp"

namespace big
{
	class humanoid : public character
	{
	public:
        using character::character;
        inventory get_inventory();
	};
}