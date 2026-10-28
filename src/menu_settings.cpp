#include "menu_settings.hpp"
#include "file_manager.hpp"
#include "nlohmann/json.hpp"
#include <filesystem>

namespace big
{
	std::filesystem::path menu_settings::get_settings_file_path() const
	{
		return file_manager::get_project_file("./menu_settings.json").get_path();
	}

	void menu_settings::attempt_save()
	{
		nlohmann::json j = *this;
		this->serialize_extra_options(j);
		if (deep_compare(this->options, j, true))
		{
			this->save();
		}
	}

	bool menu_settings::load()
	{
		this->default_options = *this;
		this->serialize_extra_options(this->default_options);
		this->options            = this->default_options;
		const auto settings_file = this->get_settings_file_path();
		try
		{
			std::ifstream file(settings_file);
			if (!file.is_open())
			{
				LOG(WARNING) << "Cannot open menu settings: " << settings_file.string() << "; using defaults.";
				return this->write_default_config();
			}
			file >> this->options;
			file.close();
			if (!this->options.is_object())
			{
				throw std::runtime_error("Menu settings root must be an object");
			}
			bool should_save = this->deep_compare(this->options, this->default_options);
			from_json(this->options, *this);
			this->deserialize_extra_options(this->options);
			if (should_save)
			{
				return this->save();
			}
		}
		catch (const std::exception& e)
		{
			LOG(WARNING) << "Invalid menu settings at " << settings_file.string() << ": " << e.what() << "; using defaults.";
			auto backup = settings_file;
			backup += ".bak";
			std::error_code backup_error;
			std::filesystem::copy_file(settings_file, backup, std::filesystem::copy_options::overwrite_existing, backup_error);
			if (backup_error)
			{
				LOG(WARNING) << "Cannot back up invalid menu settings: " << backup_error.message();
			}
			from_json(this->default_options, *this);
			this->deserialize_extra_options(this->default_options);
			this->options = this->default_options;
			return this->write_default_config();
		}
		return true;
	}

	bool menu_settings::deep_compare(nlohmann::json& current_settings, const nlohmann::json& default_settings, bool compare_value)
	{
		bool should_save = false;
		for (auto& e : default_settings.items())
		{
			const std::string& key = e.key();
			if (current_settings.count(key) == 0 || (compare_value && current_settings[key] != e.value()))
			{
				current_settings[key] = e.value();
				should_save           = true;
			}
			else if (current_settings[key].is_object() && e.value().is_object())
			{
				if (deep_compare(current_settings[key], e.value(), compare_value))
				{
					should_save = true;
				}
			}
			else if (!current_settings[key].is_object() && e.value().is_object())
			{
				current_settings[key] = e.value();
				should_save           = true;
			}
			else if (current_settings[key].size() < e.value().size())
			{
				current_settings[key] = e.value();
				should_save           = true;
			}
		}
		return should_save;
	}

	bool menu_settings::save()
	{
		const auto settings_file = this->get_settings_file_path();
		std::error_code error;
		std::filesystem::create_directories(settings_file.parent_path(), error);
		if (error)
		{
			LOG(WARNING) << "Cannot create menu settings directory: " << error.message();
			return false;
		}
		auto temp_file = settings_file;
		temp_file += ".tmp";
		std::ofstream file(temp_file, std::ios::out | std::ios::trunc);
		if (!file.is_open())
		{
			LOG(WARNING) << "Cannot open menu settings for writing: " << temp_file.string();
			return false;
		}
		nlohmann::json j = *this;
		this->serialize_extra_options(j);
		file << j.dump(4);
		file.close();
		if (file.fail())
		{
			LOG(WARNING) << "Failed to write menu settings: " << temp_file.string();
			std::filesystem::remove(temp_file, error);
			return false;
		}
		std::filesystem::rename(temp_file, settings_file, error);
		if (error)
		{
			LOG(WARNING) << "Cannot replace menu settings: " << error.message();
			std::filesystem::remove(temp_file, error);
			return false;
		}
		return true;
	}

	bool menu_settings::write_default_config()
	{
		return this->save();
	}
}
