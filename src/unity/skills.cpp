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

	static constexpr SkillType s_all_skills_list[] = {
		SkillType::Swords, SkillType::Knives, SkillType::Clubs, SkillType::Polearms,
		SkillType::Spears, SkillType::Blocking, SkillType::Axes, SkillType::Bows,
		SkillType::Crossbows, SkillType::Unarmed, SkillType::ElementalMagic, SkillType::BloodMagic,
		SkillType::Run, SkillType::Jump, SkillType::Sneak, SkillType::Swim,
		SkillType::Dodge, SkillType::Ride, SkillType::Fishing, SkillType::WoodCutting,
		SkillType::Pickaxes, SkillType::Cooking, SkillType::Farming, SkillType::Crafting
	};

	void skills::set_skill_level_direct(SkillType type, float level)
	{
		if (!m_skills)
			return;

		if (type == SkillType::All)
		{
			for (auto sk : s_all_skills_list)
				set_skill_level_direct(sk, level);
			return;
		}

		static auto get_skill_method = mono::get_method("Skills", "GetSkill", 1, "assembly_valheim");
		if (!get_skill_method)
			return;

		int skill_type = static_cast<int>(type);
		void* args[1] = {&skill_type};
		MonoObject* skill_obj = mono::invoke_method(get_skill_method, m_skills, args);
		if (!skill_obj)
			return;

		MonoClass* skill_class = mono::object_get_class(skill_obj);
		if (!skill_class)
			return;

		static MonoClassField* level_field = mono::get_field(skill_class, "m_level");
		static MonoClassField* accum_field = mono::get_field(skill_class, "m_accumulator");

		float clamped_level = std::clamp(level, 0.f, 100.f);
		if (level_field)
		{
			mono::set_field_value(skill_obj, level_field, &clamped_level);
		}
		if (accum_field)
		{
			float zero = 0.f;
			mono::set_field_value(skill_obj, accum_field, &zero);
		}
	}

	void skills::raise_skill_direct(SkillType type, float amount)
	{
		if (type == SkillType::All)
		{
			for (auto sk : s_all_skills_list)
				raise_skill_direct(sk, amount);
			return;
		}

		float current = get_skill_level_direct(type);
		set_skill_level_direct(type, current + amount);
	}

	float skills::get_skill_level_direct(SkillType type)
	{
		if (!m_skills)
			return 0.f;

		if (type == SkillType::All || type == SkillType::None)
			return 0.f;

		static auto get_skill_method = mono::get_method("Skills", "GetSkill", 1, "assembly_valheim");
		if (!get_skill_method)
			return 0.f;

		int skill_type = static_cast<int>(type);
		void* args[1] = {&skill_type};
		MonoObject* skill_obj = mono::invoke_method(get_skill_method, m_skills, args);
		if (!skill_obj)
			return 0.f;

		MonoClass* skill_class = mono::object_get_class(skill_obj);
		if (!skill_class)
			return 0.f;

		static MonoClassField* level_field = mono::get_field(skill_class, "m_level");
		if (!level_field)
			return 0.f;

		float lvl = 0.f;
		mono::get_field_value(skill_obj, level_field, &lvl);
		return lvl;
	}
}
