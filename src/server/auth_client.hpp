#pragma once
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "utility/hwid.hpp"
#include "logger/logger.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Crypt32.lib")

namespace big
{
	struct ServerAuthInfo
	{
		bool success{false};
		std::string token;
		std::string host{"apie.rena.my.id"};
		int port{443};
		bool is_ssl{true};
		std::string ws_endpoint;
		std::string error_message;
	};

	class auth_client
	{
	public:
		static ServerAuthInfo authenticate()
		{
			ServerAuthInfo info;
			resolve_server_config(info);

			std::string hwid = utils::get_hwid();
			LOG(INFO) << "[AuthClient] Generated HWID: " << hwid;
			LOG(INFO) << "[AuthClient] Target auth server: " << (info.is_ssl ? "https://" : "http://") << info.host << ":" << info.port;

			// 1. Try server-sided device login via HWID
			if (login_via_hwid(info, hwid))
			{
				info.success = true;
				info.ws_endpoint = build_ws_endpoint(info);
				LOG(INFO) << "[AuthClient] Device auto-login succeeded! Server: " << info.host << ":" << info.port;
				return info;
			}

			// 2. Fallback: try refresh_token from session.dat
			if (login_via_session_file(info))
			{
				info.success = true;
				info.ws_endpoint = build_ws_endpoint(info);
				LOG(INFO) << "[AuthClient] Auto-login via session.dat succeeded! Server: " << info.host << ":" << info.port;
				return info;
			}

			// 3. Fallback to localhost:8180 if target was remote and failed (for local dev testing)
			if (info.host != "localhost" && info.host != "127.0.0.1")
			{
				ServerAuthInfo local_info;
				local_info.host = "localhost";
				local_info.port = 8180;
				local_info.is_ssl = false;

				LOG(INFO) << "[AuthClient] Checking fallback to local dev server (localhost:8180)...";
				if (login_via_hwid(local_info, hwid) || login_via_session_file(local_info))
				{
					local_info.success = true;
					local_info.ws_endpoint = build_ws_endpoint(local_info);
					LOG(INFO) << "[AuthClient] Local dev server login succeeded!";
					return local_info;
				}
			}

			info.success = false;
			info.error_message = "Could not authenticate with Ellohim-Server. Please login via Loader first.";
			LOG(WARNING) << "[AuthClient] " << info.error_message;
			return info;
		}

	private:
		static void resolve_server_config(ServerAuthInfo& info)
		{
			// Default to production server matching Gottvergessen Loader
			info.host = "apie.rena.my.id";
			info.port = 443;
			info.is_ssl = true;

			const char* appdata = std::getenv("APPDATA");
			if (!appdata) return;

			std::filesystem::path config_path = std::filesystem::path(appdata) / "Ellohim Menu" / "Config" / "environment.json";
			if (!std::filesystem::exists(config_path)) return;

			try
			{
				std::ifstream f(config_path);
				auto j = nlohmann::json::parse(f, nullptr, false);
				if (j.is_discarded()) return;

				int env_type = j.value("env_type", 1);
				std::string custom_url = j.value("custom_url", "");

				if (env_type == 1) // PRODUCTION
				{
					info.host = "apie.rena.my.id";
					info.port = 443;
					info.is_ssl = true;
				}
				else if (env_type == 2 && !custom_url.empty()) // CUSTOM
				{
					parse_url(custom_url, info);
				}
				else if (env_type == 0) // LOCAL
				{
					info.host = "localhost";
					info.port = 8180;
					info.is_ssl = false;
				}
			}
			catch (...) {}
		}

		static void parse_url(const std::string& url, ServerAuthInfo& info)
		{
			std::string target = url;
			if (target.find("https://") == 0)
			{
				info.is_ssl = true;
				info.port = 443;
				target = target.substr(8);
			}
			else if (target.find("http://") == 0)
			{
				info.is_ssl = false;
				info.port = 80;
				target = target.substr(7);
			}

			auto slash_pos = target.find('/');
			if (slash_pos != std::string::npos)
			{
				target = target.substr(0, slash_pos);
			}

			auto colon_pos = target.find(':');
			if (colon_pos != std::string::npos)
			{
				info.host = target.substr(0, colon_pos);
				try
				{
					info.port = std::stoi(target.substr(colon_pos + 1));
				}
				catch (...) {}
			}
			else
			{
				info.host = target;
			}
		}

		static std::string build_ws_endpoint(const ServerAuthInfo& info)
		{
			std::string endpoint = info.host;
			if ((info.is_ssl && info.port != 443) || (!info.is_ssl && info.port != 80))
			{
				endpoint += ":" + std::to_string(info.port);
			}
			return endpoint + "/ws/auth?token=" + info.token + "&client=valheim_mod";
		}

		static bool login_via_hwid(ServerAuthInfo& info, const std::string& hwid)
		{
			nlohmann::json body = {{"hwid", hwid}};
			std::string response_body;
			int status_code = 0;

			if (!http_post(info, "/auth/device-login", body.dump(), "", response_body, status_code))
			{
				LOG(WARNING) << "[AuthClient] HWID login HTTP request failed (server unreachable at " << info.host << ":" << info.port << ")";
				return false;
			}

			if (status_code == 200 && !response_body.empty())
			{
				auto j = nlohmann::json::parse(response_body, nullptr, false);
				if (!j.is_discarded() && j.value("success", false) && j.contains("data") && j["data"].contains("token"))
				{
					info.token = j["data"]["token"].get<std::string>();
					return !info.token.empty();
				}
			}
			else
			{
				LOG(INFO) << "[AuthClient] HWID not registered on server yet (status " << status_code << "). Falling back to session.dat...";
			}
			return false;
		}

		static bool login_via_session_file(ServerAuthInfo& info)
		{
			const char* appdata = std::getenv("APPDATA");
			if (!appdata)
			{
				LOG(WARNING) << "[AuthClient] APPDATA environment variable not found";
				return false;
			}

			std::filesystem::path dat_path = std::filesystem::path(appdata) / "Ellohim Menu" / "Config" / "session.dat";
			if (!std::filesystem::exists(dat_path))
			{
				LOG(WARNING) << "[AuthClient] session.dat not found at: " << dat_path.string();
				return false;
			}

			try
			{
				std::ifstream file(dat_path, std::ios::binary | std::ios::ate);
				auto size = file.tellg();
				if (size <= 0)
				{
					LOG(WARNING) << "[AuthClient] session.dat is empty";
					return false;
				}

				std::vector<uint8_t> cipher(static_cast<size_t>(size));
				file.seekg(0, std::ios::beg);
				file.read(reinterpret_cast<char*>(cipher.data()), size);
				file.close();

				DATA_BLOB input{ static_cast<DWORD>(cipher.size()), cipher.data() };
				DATA_BLOB output{};
				std::string refresh_token;

				if (CryptUnprotectData(&input, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &output))
				{
					std::string plain(reinterpret_cast<char*>(output.pbData), output.cbData);
					LocalFree(output.pbData);

					auto j = nlohmann::json::parse(plain, nullptr, false);
					if (!j.is_discarded())
					{
						refresh_token = j.value("refresh_token", "");
					}
				}
				else
				{
					LOG(WARNING) << "[AuthClient] CryptUnprotectData failed: " << GetLastError();
					return false;
				}

				if (refresh_token.empty())
				{
					LOG(WARNING) << "[AuthClient] refresh_token in session.dat is empty";
					return false;
				}

				std::string cookie_header = "Cookie: refresh_token=" + refresh_token;
				std::string response_body;
				int status_code = 0;

				if (!http_post(info, "/auth/refresh", "", cookie_header, response_body, status_code))
				{
					LOG(WARNING) << "[AuthClient] Refresh token HTTP request failed (server unreachable)";
					return false;
				}

				if (status_code == 200 && !response_body.empty())
				{
					auto j = nlohmann::json::parse(response_body, nullptr, false);
					if (!j.is_discarded() && j.value("success", false) && j.contains("data") && j["data"].contains("token"))
					{
						info.token = j["data"]["token"].get<std::string>();
						return !info.token.empty();
					}
				}
				else
				{
					LOG(WARNING) << "[AuthClient] Session refresh returned status " << status_code << ": " << response_body;
				}
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << "[AuthClient] Exception reading session.dat: " << e.what();
			}
			catch (...)
			{
				LOG(WARNING) << "[AuthClient] Unknown exception reading session.dat";
			}

			return false;
		}

		static bool http_post(const ServerAuthInfo& info, const std::string& path, const std::string& body, const std::string& extra_headers, std::string& out_response, int& out_status)
		{
			HINTERNET hSession = WinHttpOpen(L"MonoHacking/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!hSession)
			{
				LOG(WARNING) << "[AuthClient] WinHttpOpen failed: " << GetLastError();
				return false;
			}

			WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

			std::wstring wHost(info.host.begin(), info.host.end());
			HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), static_cast<INTERNET_PORT>(info.port), 0);
			if (!hConnect)
			{
				LOG(WARNING) << "[AuthClient] WinHttpConnect failed to " << info.host << ":" << info.port << ", error: " << GetLastError();
				WinHttpCloseHandle(hSession);
				return false;
			}

			std::wstring wPath(path.begin(), path.end());
			DWORD reqFlags = (info.is_ssl ? WINHTTP_FLAG_SECURE : 0) | WINHTTP_FLAG_REFRESH;
			HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, reqFlags);
			if (!hRequest)
			{
				LOG(WARNING) << "[AuthClient] WinHttpOpenRequest failed: " << GetLastError();
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);
				return false;
			}

			// If SSL, ignore cert errors for dev/custom environments
			if (info.is_ssl)
			{
				DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
				WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
			}

			// Add extra headers first (e.g. Cookie)
			if (!extra_headers.empty())
			{
				std::wstring wExtra(extra_headers.begin(), extra_headers.end());
				if (wExtra.find(L"\r\n") == std::wstring::npos)
				{
					wExtra += L"\r\n";
				}
				WinHttpAddRequestHeaders(hRequest, wExtra.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
			}

			std::wstring default_headers = L"Content-Type: application/json\r\nAccept: application/json\r\n";
			WinHttpAddRequestHeaders(hRequest, default_headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

			BOOL bSend = WinHttpSendRequest(
				hRequest,
				WINHTTP_NO_ADDITIONAL_HEADERS,
				0,
				body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
				static_cast<DWORD>(body.length()),
				static_cast<DWORD>(body.length()),
				0
			);

			if (!bSend)
			{
				LOG(WARNING) << "[AuthClient] WinHttpSendRequest failed: " << GetLastError();
				WinHttpCloseHandle(hRequest);
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);
				return false;
			}

			if (!WinHttpReceiveResponse(hRequest, NULL))
			{
				LOG(WARNING) << "[AuthClient] WinHttpReceiveResponse failed: " << GetLastError();
				WinHttpCloseHandle(hRequest);
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);
				return false;
			}

			DWORD statusCode = 0;
			DWORD statusSize = sizeof(statusCode);
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
			out_status = static_cast<int>(statusCode);

			DWORD bytesAvailable = 0;
			std::string respData;
			while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
			{
				std::vector<char> buf(bytesAvailable + 1);
				DWORD bytesRead = 0;
				if (WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead) && bytesRead > 0)
				{
					respData.append(buf.data(), bytesRead);
				}
			}

			out_response = std::move(respData);

			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return true;
		}
	};
}


