#pragma once
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <wincrypt.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Crypt32.lib")

namespace utils
{
	inline std::string get_hwid()
	{
		std::string raw_id;

		// 1. Windows MachineGuid from Registry
		HKEY hKey;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
		{
			char buffer[256]{};
			DWORD size = sizeof(buffer);
			if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS)
			{
				raw_id += buffer;
			}
			RegCloseKey(hKey);
		}

		// 2. Volume Serial Number for C: drive
		DWORD volumeSerial = 0;
		if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0))
		{
			raw_id += "-" + std::to_string(volumeSerial);
		}

		if (raw_id.empty())
		{
			raw_id = "UNKNOWN_HWID_MACHINE";
		}

		// 3. SHA-256 Hash of raw_id
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		std::string hash_hex;

		if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
			{
				CryptHashData(hHash, reinterpret_cast<const BYTE*>(raw_id.data()), static_cast<DWORD>(raw_id.size()), 0);
				BYTE hash[32];
				DWORD hashLen = sizeof(hash);
				if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
				{
					std::ostringstream oss;
					for (DWORD i = 0; i < hashLen; ++i)
					{
						oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
					}
					hash_hex = oss.str();
				}
				CryptDestroyHash(hHash);
			}
			CryptReleaseContext(hProv, 0);
		}

		return hash_hex.empty() ? raw_id : hash_hex;
	}
}

