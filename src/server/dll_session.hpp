#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <winhttp.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "utility/hwid.hpp"
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Crypt32.lib")

namespace big
{
	class dll_session_client
	{
	public:
		dll_session_client()
		{
			const auto bootstrap = read_bootstrap();
			if (!bootstrap)
				throw std::runtime_error("DLL session ticket missing or expired. Launch this game through Loader again.");
			m_hwid = utils::get_hwid();
			const auto server = load_config();
			const nlohmann::json request{{"hwid", m_hwid}, {"ticket", bootstrap->ticket}, {"pid", bootstrap->pid}, {"process_started", bootstrap->process_started}, {"binary_id", bootstrap->binary_id}};
			std::string response;
			int status{};
			if (!post(server, "/dll-session/create", request.dump(), {}, response, status) || status != 200)
				throw std::runtime_error("DLL license validation failed.");
			const auto json = nlohmann::json::parse(response, nullptr, false);
			const auto* data = json.is_object() && json.contains("data") && json["data"].is_object() ? &json["data"] : &json;
			if (json.is_discarded() || !json.value("success", false) || !data->contains("token") || !(*data)["token"].is_string())
				throw std::runtime_error("DLL license response was invalid.");
			m_token = (*data)["token"].get<std::string>();
			const auto lease = data->value("lease_seconds", 0);
			if (m_token.size() != 64 || m_token.find_first_not_of("0123456789abcdef") != std::string::npos || lease <= 0)
				throw std::runtime_error("DLL license lease was invalid.");
			m_lease_deadline = clock::now() + std::chrono::seconds(lease);
			m_thread = std::thread([this, server] {
				heartbeat_loop(server);
			});
		}

		~dll_session_client() noexcept
		{
			m_stop = true;
			if (m_thread.joinable())
				m_thread.join();
			const auto server = load_config();
			std::string response;
			int status{};
			post(server, "/dll-session/close", "{}", {{"Authorization", "Bearer " + m_token}}, response, status);
			if (!m_token.empty())
				SecureZeroMemory(m_token.data(), m_token.size());
		}

		dll_session_client(const dll_session_client&) = delete;
		dll_session_client& operator=(const dll_session_client&) = delete;

		bool authorized() const
		{
			std::lock_guard lock(m_mutex);
			return !m_revoked && clock::now() < m_lease_deadline;
		}

	private:
		using clock = std::chrono::steady_clock;
		struct config
		{
			std::wstring host{L"apie.rena.my.id"};
			INTERNET_PORT port{443};
			bool secure{true};
		};
		struct bootstrap
		{
			std::string ticket;
			unsigned long pid{};
			std::string process_started;
			std::string binary_id;
		};

		static config load_config()
		{
			config result;
			const auto* appdata = std::getenv("APPDATA");
			if (!appdata)
				return result;
			std::ifstream file(std::filesystem::path(appdata) / "Ellohim Menu" / "Config" / "environment.json");
			const auto json = nlohmann::json::parse(file, nullptr, false);
			if (json.is_discarded())
				return result;
			const auto url = json.value("custom_url", std::string{});
			if (json.value("env_type", 1) == 0)
			{
				result.host = L"localhost";
				result.port = 8180;
				result.secure = false;
			}
			else if (json.value("env_type", 1) == 2 && !url.empty())
			{
				auto value = url;
				result.secure = value.rfind("https://", 0) == 0;
				value = value.substr(result.secure ? 8u : (value.rfind("http://", 0) == 0 ? 7u : 0u));
				if (const auto slash = value.find('/'); slash != std::string::npos)
					value.resize(slash);
				if (const auto colon = value.find(':'); colon != std::string::npos)
				{
					result.port = static_cast<INTERNET_PORT>(std::stoi(value.substr(colon + 1)));
					value.resize(colon);
				}
				result.host = std::wstring(value.begin(), value.end());
			}
			return result;
		}

		static std::optional<bootstrap> read_bootstrap()
		{
			wchar_t image[32768]{};
			const auto length = GetModuleFileNameW(nullptr, image, std::size(image));
			if (!length || length >= std::size(image))
				return std::nullopt;
			FILETIME created{}, exited{}, kernel{}, user{};
			if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
				return std::nullopt;
			const auto started = std::to_string((static_cast<std::uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime);
			const auto pid = GetCurrentProcessId();
			const auto path = std::filesystem::path(image).parent_path() / ".astra" / ("session-" + std::to_string(pid) + ".dat");
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file)
				return std::nullopt;
			const auto size = file.tellg();
			if (size <= 0 || size > 4096)
				return std::nullopt;
			std::vector<std::uint8_t> encrypted(static_cast<size_t>(size));
			file.seekg(0);
			if (!file.read(reinterpret_cast<char*>(encrypted.data()), size))
				return std::nullopt;
			file.close();
			std::error_code error;
			std::filesystem::remove(path, error);
			DATA_BLOB input{static_cast<DWORD>(encrypted.size()), encrypted.data()}, output{};
			if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
				return std::nullopt;
			std::string plain(reinterpret_cast<char*>(output.pbData), output.cbData);
			SecureZeroMemory(output.pbData, output.cbData);
			LocalFree(output.pbData);
			const auto json = nlohmann::json::parse(plain, nullptr, false);
			SecureZeroMemory(plain.data(), plain.size());
			if (json.is_discarded() || json.value("version", 0) != 1 || !json.contains("ticket") || !json["ticket"].is_string() || !json.contains("pid") || !json["pid"].is_number_unsigned() || !json.contains("process_started") || !json["process_started"].is_string() || !json.contains("binary_id") || !json["binary_id"].is_string())
				return std::nullopt;
			bootstrap result{json["ticket"].get<std::string>(), json["pid"].get<unsigned long>(), json["process_started"].get<std::string>(), json["binary_id"].get<std::string>()};
			if (result.pid != pid || result.process_started != started || result.binary_id.empty() || result.ticket.size() != 64 || result.ticket.find_first_not_of("0123456789abcdef") != std::string::npos)
				return std::nullopt;
			return result;
		}
		static bool post(const config& server, const std::string& path, const std::string& body,
		    const std::vector<std::pair<std::string, std::string>>& headers,
		    std::string& response, int& status)
		{
			const auto session = WinHttpOpen(L"Ellohim-DLL/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!session)
				return false;
			WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
			const auto connection = WinHttpConnect(session, server.host.c_str(), server.port, 0);
			if (!connection)
			{
				WinHttpCloseHandle(session);
				return false;
			}
			const auto wide_path = std::wstring(path.begin(), path.end());
			const auto request = WinHttpOpenRequest(connection, L"POST", wide_path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, server.secure ? WINHTTP_FLAG_SECURE : 0);
			if (!request)
			{
				WinHttpCloseHandle(connection);
				WinHttpCloseHandle(session);
				return false;
			}
			std::wstring header_text = L"Content-Type: application/json\r\nAccept: application/json\r\n";
			for (const auto& [name, value] : headers)
			{
				header_text += std::wstring(name.begin(), name.end()) + L": " + std::wstring(value.begin(), value.end()) + L"\r\n";
			}
			WinHttpAddRequestHeaders(request, header_text.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
			const auto sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
			const auto received = sent && WinHttpReceiveResponse(request, nullptr);
			if (received)
			{
				DWORD status_code{}, status_size = sizeof(status_code);
				WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_size, WINHTTP_NO_HEADER_INDEX);
				status = static_cast<int>(status_code);
				DWORD available{};
				while (WinHttpQueryDataAvailable(request, &available) && available)
				{
					std::vector<char> buffer(available);
					DWORD read{};
					if (WinHttpReadData(request, buffer.data(), available, &read) && read)
						response.append(buffer.data(), read);
				}
			}
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connection);
			WinHttpCloseHandle(session);
			return received;
		}

		void heartbeat_loop(const config& server) noexcept
		{
			while (!m_stop)
			{
				std::this_thread::sleep_for(std::chrono::seconds(20));
				if (m_stop)
					break;
				std::string response;
				int status{};
				const auto ok = post(server, "/dll-session/heartbeat", "{}", {{"Authorization", "Bearer " + m_token}, {"X-HWID", m_hwid}}, response, status);
				const auto json = nlohmann::json::parse(response, nullptr, false);
				const auto* data = json.is_object() && json.contains("data") && json["data"].is_object() ? &json["data"] : &json;
				if (ok && status == 200 && !json.is_discarded() && json.value("success", false) && data->value("lease_seconds", 0) > 0)
				{
					std::lock_guard lock(m_mutex);
					m_lease_deadline = clock::now() + std::chrono::seconds(data->value("lease_seconds", 0));
				}
				else if (status == 401 || status == 403)
				{
					std::lock_guard lock(m_mutex);
					m_revoked = true;
					break;
				}
			}
		}

		std::string m_hwid;
		std::string m_token;
		std::thread m_thread;
		std::atomic_bool m_stop{false};
		mutable std::mutex m_mutex;
		clock::time_point m_lease_deadline{};
		bool m_revoked{false};
	};
}
