#include "menu_settings.hpp"
#include "nlohmann/json.hpp"
#include <filesystem>

namespace big
{
    void menu_settings::attempt_save()
    {
        nlohmann::json j = *this;
        if (deep_compare(this->options, j, true))
            this->save();
    }

    bool menu_settings::load()
    {
        this->default_options = *this;
        this->options = this->default_options;
        const char* appdata = std::getenv("appdata");
        if (!appdata)
        {
            LOG(WARNING) << "APPDATA is unavailable; using menu defaults in memory.";
            return false;
        }
        const std::string settings_file = std::string(appdata) + this->settings_location;
        try
        {
            std::ifstream file(settings_file);
            if (!file.is_open())
            {
                LOG(WARNING) << "Cannot open menu settings: " << settings_file << "; using defaults.";
                return this->write_default_config();
            }
            file >> this->options;
            file.close();
            if (!this->options.is_object())
                throw std::runtime_error("Menu settings root must be an object");
            bool should_save = this->deep_compare(this->options, this->default_options);
            from_json(this->options, *this);
            if (should_save)
                return this->save();
        }
        catch (const std::exception& e)
        {
            LOG(WARNING) << "Invalid menu settings at " << settings_file << ": " << e.what() << "; using defaults.";
            from_json(this->default_options, *this);
            this->options = this->default_options;
            return this->write_default_config();
        }
        return true;
    }

    bool menu_settings::deep_compare(nlohmann::json &current_settings, const nlohmann::json &default_settings, bool compare_value)
    {
        bool should_save = false;
        for (auto& e : default_settings.items())
        {
            const std::string& key = e.key();
            if (current_settings.count(key) == 0 || (compare_value && current_settings[key] != e.value()))
            {
                current_settings[key] = e.value();
                should_save = true;
            }
            else if (current_settings[key].is_object() && e.value().is_object())
            {
                if (deep_compare(current_settings[key], e.value(), compare_value))
                    should_save = true;
            }
            else if (!current_settings[key].is_object() && e.value().is_object())
            {
                current_settings[key] = e.value();
                should_save = true;
            }
            else if (current_settings[key].size() < e.value().size())
            {
                current_settings[key] = e.value();
                should_save = true;
            }
        }
        return should_save;
    }

    bool menu_settings::save()
    {
        const char* appdata = std::getenv("appdata");
        if (!appdata)
        {
            LOG(WARNING) << "Cannot save menu settings: APPDATA is unavailable.";
            return false;
        }
        const std::string settings_file = std::string(appdata) + this->settings_location;
        std::error_code error;
        std::filesystem::create_directories(std::filesystem::path(settings_file).parent_path(), error);
        if (error)
        {
            LOG(WARNING) << "Cannot create menu settings directory: " << error.message();
            return false;
        }
        std::ofstream file(settings_file, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            LOG(WARNING) << "Cannot open menu settings for writing: " << settings_file;
            return false;
        }
        nlohmann::json j = *this;
        file << j.dump(4);
        file.close();
        if (file.fail())
        {
            LOG(WARNING) << "Failed to write menu settings: " << settings_file;
            return false;
        }
        return true;
    }

    bool menu_settings::write_default_config()
    {
        return this->save();
    }
}
