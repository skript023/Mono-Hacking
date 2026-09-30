#include "skills.hpp"
#include "utility/unity.hpp"

namespace big
{
	skills::skills(MonoObject* o) :
	    m_skills(o)
	{
	}

	skills::~skills() noexcept
	{
		m_skills = nullptr;
	}

	mono_array_view<skill_def> skills::get_all_skill_def()
	{
		if (!m_skills)
			return {};

		auto s = mono::get_field_value<"Skills", "m_skills", MonoObject*>(m_skills);

		return mono::list<skill_def>(s);
	}

	void skills::cheat_raise_skill(const std::string& name, float value, bool show_message)
	{
		static auto method = mono::get_method("Skills", "CheatRaiseSkill", 3, "assembly_valheim");
		if (!method || !m_skills)
			return;

		auto ms = mono::to_mono_string(name);
		void* args[3] = {ms, &value, &show_message};
		mono::invoke_method(method, m_skills, args);
	}

	void skills::cheat_raise_skill(SkillType type, float value, bool show_message)
	{
		cheat_raise_skill(std::string(skill_type_to_csharp_name(type)), value, show_message);
	}

	void skills::cheat_reset_skill(const std::string& name)
	{
		static auto method = mono::get_method("Skills", "CheatResetSkill", 1, "assembly_valheim");
		if (!method || !m_skills)
			return;

		auto ms = mono::to_mono_string(name);
		void* args[1] = {ms};
		mono::invoke_method(method, m_skills, args);
	}

	void skills::cheat_reset_skill(SkillType type)
	{
		cheat_reset_skill(std::string(skill_type_to_csharp_name(type)));
	}

	float skills::get_skill_level(SkillType type)
	{
		static auto method = mono::get_method("Skills", "GetSkillLevel", 1, "assembly_valheim");
		if (!method || !m_skills)
			return 0.f;

		int skill_type = static_cast<int>(type);
		void* args[1] = {&skill_type};
		auto obj = mono::invoke_method(method, m_skills, args);
		if (!obj)
			return 0.f;

		return *reinterpret_cast<float*>(mono::object_unbox(obj));
	}

	skills skills::get_local_skills()
	{
		auto player_obj = unity::get_local_player();
		if (!player_obj)
			return nullptr;

		static auto method = mono::get_method("Player", "GetSkills", 0, "assembly_valheim");
		if (!method)
			return nullptr;

		auto obj = mono::invoke(method, player_obj);
		if (!obj)
			return nullptr;

		return obj;
	}

	std::string_view skills::skill_type_to_csharp_name(SkillType type)
	{
		switch (type)
		{
		case SkillType::Swords: return "Swords";
		case SkillType::Knives: return "Knives";
		case SkillType::Clubs: return "Clubs";
		case SkillType::Polearms: return "Polearms";
		case SkillType::Spears: return "Spears";
		case SkillType::Blocking: return "Blocking";
		case SkillType::Axes: return "Axes";
		case SkillType::Bows: return "Bows";
		case SkillType::ElementalMagic: return "ElementalMagic";
		case SkillType::BloodMagic: return "BloodMagic";
		case SkillType::Unarmed: return "Unarmed";
		case SkillType::Pickaxes: return "Pickaxes";
		case SkillType::WoodCutting: return "WoodCutting";
		case SkillType::Crossbows: return "Crossbows";
		case SkillType::Jump: return "Jump";
		case SkillType::Sneak: return "Sneak";
		case SkillType::Run: return "Run";
		case SkillType::Swim: return "Swim";
		case SkillType::Fishing: return "Fishing";
		case SkillType::Cooking: return "Cooking";
		case SkillType::Farming: return "Farming";
		case SkillType::Crafting: return "Crafting";
		case SkillType::Dodge: return "Dodge";
		case SkillType::Ride: return "Ride";
		case SkillType::All: return "All";
		default: return "None";
		}
	}

	std::string_view skills::skill_type_to_display_name(SkillType type)
	{
		switch (type)
		{
		case SkillType::Swords: return "Swords";
		case SkillType::Knives: return "Knives";
		case SkillType::Clubs: return "Clubs";
		case SkillType::Polearms: return "Polearms";
		case SkillType::Spears: return "Spears";
		case SkillType::Blocking: return "Blocking";
		case SkillType::Axes: return "Axes";
		case SkillType::Bows: return "Bows";
		case SkillType::ElementalMagic: return "Elemental Magic";
		case SkillType::BloodMagic: return "Blood Magic";
		case SkillType::Unarmed: return "Unarmed";
		case SkillType::Pickaxes: return "Pickaxes";
		case SkillType::WoodCutting: return "Wood Cutting";
		case SkillType::Crossbows: return "Crossbows";
		case SkillType::Jump: return "Jump";
		case SkillType::Sneak: return "Sneak";
		case SkillType::Run: return "Run";
		case SkillType::Swim: return "Swim";
		case SkillType::Fishing: return "Fishing";
		case SkillType::Cooking: return "Cooking";
		case SkillType::Farming: return "Farming";
		case SkillType::Crafting: return "Crafting";
		case SkillType::Dodge: return "Dodge";
		case SkillType::Ride: return "Ride";
		case SkillType::All: return "All Skills";
		default: return "None";
		}
	}
}
