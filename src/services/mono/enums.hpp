#pragma once

namespace big
{
	enum class SkillType : int32_t
	{
		None,
		Swords,
		Knives,
		Clubs,
		Polearms,
		Spears,
		Blocking,
		Axes,
		Bows,
		ElementalMagic,
		BloodMagic,
		Unarmed,
		Pickaxes,
		WoodCutting,
		CrossBows,
		Jump = 100,
		Sneak,
		Run,
		Swim,
		Fishing,
		Cooking,
		Farming,
		Crafting,
		Dodge,
		Ride = 110,
		All = 999
	};
}