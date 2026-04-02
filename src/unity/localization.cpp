#include "localization.hpp"

namespace big
{
	localization::localization(MonoObject* obj): m_localization(obj)
	{}
	localization::~localization() noexcept
	{
        m_localization = nullptr;
	}
	std::string localization::Localize(std::string const& text)
	{
		auto method = mono::get_method("Localization", "Localize", 1, "assembly_guiutils");

        if (!method)
        {
            LOG(FATAL) << "Method not found";
        }

        auto ms = mono::to_mono_string(text);

        auto ret = mono::invoke(method, m_localization, ms);

        return mono::from_mono_string(reinterpret_cast<MonoString*>(ret));
	}
}