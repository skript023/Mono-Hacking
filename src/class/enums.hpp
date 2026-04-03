#pragma once

enum class EEntityType
{
    Player,
    Character,
    ItemDrop,
    Pickable
};

enum SkillType : int
{
    None = 0,
    Swords = 1,
    Knives = 2,
    Clubs = 3,
    Polearms = 4,
    Spears = 5,
    Blocking = 6,
    Axes = 7,
    Bows = 8,
    ElementalMagic = 9,
    BloodMagic = 10,
    Unarmed = 11,
    Pickaxes = 12,
    WoodCutting = 13,
    Crossbows = 14,
    Jump = 100,
    Sneak = 101,
    Run = 102,
    Swim = 103,
    Fishing = 104,
    Cooking = 105,
    Farming = 106,
    Crafting = 107,
    Dodge = 108,
    Ride = 110,
    All = 999
};