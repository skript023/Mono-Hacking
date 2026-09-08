#pragma once
#include <windows.h>
#include <winioctl.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <wincrypt.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

namespace utils
{
#pragma pack(push, 1)
	struct RawSMBIOSData
	{
		BYTE  Used20CallingMethod;
		BYTE  SMBIOSMajorVersion;
		BYTE  SMBIOSMinorVersion;
		BYTE  DmiRevision;
		DWORD Length;
		BYTE  SMBIOSTableData[1];
	};

	struct SMBIOSHeader
	{
		BYTE Type;
		BYTE Length;
		WORD Handle;
	};
#pragma pack(pop)

	inline std::string get_computer_name()
	{
		char buffer[MAX_COMPUTERNAME_LENGTH + 1]{};
		DWORD size = sizeof(buffer);
		if (GetComputerNameA(buffer, &size))
		{
			std::string name(buffer);
			name.erase(0, name.find_first_not_of(" \t\r\n"));
			name.erase(name.find_last_not_of(" \t\r\n") + 1);
			if (!name.empty())
				return name;
		}
		return "Desktop-PC";
	}

	inline std::string get_device_name()
	{
		return get_computer_name();
	}

	inline std::string get_machine_guid()
	{
		std::string guid;
		HKEY hKey;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
		{
			char buffer[256]{};
			DWORD size = sizeof(buffer);
			if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS)
			{
				guid = buffer;
			}
			RegCloseKey(hKey);
		}
		return guid;
	}

	inline void get_smbios_data(std::string& system_uuid, std::string& mb_serial)
	{
		system_uuid.clear();
		mb_serial.clear();

		// 'RSMB' provider signature: 0x52534D42
		const DWORD rsmb = 0x52534D42;
		DWORD bufSize = GetSystemFirmwareTable(rsmb, 0, nullptr, 0);
		if (bufSize == 0)
			return;

		std::vector<BYTE> buffer(bufSize);
		if (GetSystemFirmwareTable(rsmb, 0, buffer.data(), bufSize) == 0)
			return;

		auto* pData = reinterpret_cast<RawSMBIOSData*>(buffer.data());
		const BYTE* ptr = pData->SMBIOSTableData;
		const BYTE* endPtr = ptr + pData->Length;

		while (ptr + sizeof(SMBIOSHeader) <= endPtr)
		{
			const auto* header = reinterpret_cast<const SMBIOSHeader*>(ptr);
			if (header->Length < sizeof(SMBIOSHeader) || ptr + header->Length > endPtr)
				break;

			const BYTE* strPtr = ptr + header->Length;

			auto get_string = [&](BYTE index) -> std::string {
				if (index == 0) return "";
				const char* s = reinterpret_cast<const char*>(strPtr);
				BYTE current = 1;
				while (reinterpret_cast<const BYTE*>(s) < endPtr && *s != '\0')
				{
					if (current == index)
						return std::string(s);
					s += strlen(s) + 1;
					current++;
				}
				return "";
			};

			// Type 1: System Information (Motherboard / System UUID)
			if (header->Type == 1 && header->Length >= 0x19 && system_uuid.empty())
			{
				const BYTE* u = ptr + 0x08;
				bool allZero = true;
				bool allFF = true;
				for (int i = 0; i < 16; ++i)
				{
					if (u[i] != 0x00) allZero = false;
					if (u[i] != 0xFF) allFF = false;
				}
				if (!allZero && !allFF)
				{
					char uuidStr[64]{};
					sprintf_s(uuidStr, sizeof(uuidStr),
						"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
						u[3], u[2], u[1], u[0],
						u[5], u[4],
						u[7], u[6],
						u[8], u[9],
						u[10], u[11], u[12], u[13], u[14], u[15]);
					system_uuid = uuidStr;
				}
			}
			// Type 2: Base Board (Motherboard Serial Number)
			else if (header->Type == 2 && header->Length >= 0x08 && mb_serial.empty())
			{
				BYTE serialIdx = ptr[0x07];
				std::string s = get_string(serialIdx);
				s.erase(0, s.find_first_not_of(" \t\r\n"));
				s.erase(s.find_last_not_of(" \t\r\n") + 1);
				if (!s.empty() && s != "Default string" && s != "None" && s.find("O.E.M.") == std::string::npos)
				{
					mb_serial = s;
				}
			}

			// Advance past formatted area and null-terminated string table
			const BYTE* nextPtr = strPtr;
			while (nextPtr + 1 < endPtr)
			{
				if (*nextPtr == 0 && *(nextPtr + 1) == 0)
				{
					nextPtr += 2;
					break;
				}
				nextPtr++;
			}
			if (nextPtr <= ptr)
				break;
			ptr = nextPtr;
		}
	}

	inline std::string get_physical_drive_serial()
	{
		for (int driveIdx = 0; driveIdx < 4; ++driveIdx)
		{
			std::wstring drivePath = L"\\\\.\\PhysicalDrive" + std::to_wstring(driveIdx);
			HANDLE hDevice = CreateFileW(
				drivePath.c_str(),
				0, // Query access only (does NOT require administrator privileges)
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				0,
				nullptr);

			if (hDevice != INVALID_HANDLE_VALUE)
			{
				STORAGE_PROPERTY_QUERY query{};
				query.PropertyId = StorageDeviceProperty;
				query.QueryType = PropertyStandardQuery;

				BYTE buffer[1024]{};
				DWORD bytesReturned = 0;

				if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
									&query, sizeof(query),
									buffer, sizeof(buffer),
									&bytesReturned, nullptr))
				{
					auto* desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
					if (desc->SerialNumberOffset != 0 && desc->SerialNumberOffset < bytesReturned)
					{
						const char* rawSerial = reinterpret_cast<const char*>(buffer + desc->SerialNumberOffset);
						std::string serial(rawSerial);
						serial.erase(0, serial.find_first_not_of(" \t\r\n"));
						serial.erase(serial.find_last_not_of(" \t\r\n") + 1);
						CloseHandle(hDevice);
						if (!serial.empty() && serial.find_first_not_of("0 ") != std::string::npos)
						{
							return serial;
						}
					}
				}
				CloseHandle(hDevice);
			}
		}

		// Fallback to Volume Serial Number for C: drive if direct IOCTL query is not accessible
		DWORD volumeSerial = 0;
		if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0))
		{
			return std::to_string(volumeSerial);
		}
		return "";
	}

	inline std::string get_cpuid_info()
	{
		int cpuInfo[4] = { 0 };
		__cpuid(cpuInfo, 1);

		// Mask out bits 24-31 of EBX (Initial APIC ID) because it varies depending
		// on which CPU core the OS thread scheduler runs on at that millisecond!
		int ebx_clean = cpuInfo[1] & 0x00FFFFFF;

		std::ostringstream oss;
		oss << std::hex << std::setfill('0');
		oss << std::setw(8) << cpuInfo[0] << "-"
			<< std::setw(8) << ebx_clean << "-"
			<< std::setw(8) << cpuInfo[2] << "-"
			<< std::setw(8) << cpuInfo[3];

		__cpuid(cpuInfo, 0x80000000);
		unsigned int nExIds = static_cast<unsigned int>(cpuInfo[0]);
		if (nExIds >= 0x80000004)
		{
			int brandBuf[12]{};
			__cpuid(brandBuf, 0x80000002);
			__cpuid(brandBuf + 4, 0x80000003);
			__cpuid(brandBuf + 8, 0x80000004);

			char brand[49]{};
			memcpy_s(brand, sizeof(brand), brandBuf, 48);
			brand[48] = '\0';

			std::string brandStr(brand);
			brandStr.erase(0, brandStr.find_first_not_of(" \t\r\n"));
			brandStr.erase(brandStr.find_last_not_of(" \t\r\n") + 1);
			if (!brandStr.empty())
			{
				oss << "-" << brandStr;
			}
		}
		return oss.str();
	}

	inline std::string get_physical_mac()
	{
		ULONG outBufLen = 15000;
		std::vector<BYTE> buffer(outBufLen);
		auto* pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

		ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES;
		DWORD ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &outBufLen);
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			buffer.resize(outBufLen);
			pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
			ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &outBufLen);
		}

		if (ret == NO_ERROR)
		{
			for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next)
			{
				if (pCurr->IfType != IF_TYPE_ETHERNET_CSMACD && pCurr->IfType != IF_TYPE_IEEE80211)
					continue;

				if (pCurr->PhysicalAddressLength != 6)
					continue;

				std::wstring desc = pCurr->Description ? pCurr->Description : L"";
				std::wstring friendly = pCurr->FriendlyName ? pCurr->FriendlyName : L"";
				std::wstring combined = desc + L" " + friendly;
				std::transform(combined.begin(), combined.end(), combined.begin(), ::towlower);

				if (combined.find(L"virtual") != std::wstring::npos ||
					combined.find(L"vmware") != std::wstring::npos ||
					combined.find(L"virtualbox") != std::wstring::npos ||
					combined.find(L"hyper-v") != std::wstring::npos ||
					combined.find(L"tap") != std::wstring::npos ||
					combined.find(L"vpn") != std::wstring::npos ||
					combined.find(L"npcap") != std::wstring::npos ||
					combined.find(L"loopback") != std::wstring::npos ||
					combined.find(L"bluetooth") != std::wstring::npos ||
					combined.find(L"wsl") != std::wstring::npos ||
					combined.find(L"host-only") != std::wstring::npos)
				{
					continue;
				}

				bool nonZero = false;
				for (ULONG i = 0; i < pCurr->PhysicalAddressLength; ++i)
				{
					if (pCurr->PhysicalAddress[i] != 0)
					{
						nonZero = true;
						break;
					}
				}
				if (!nonZero)
					continue;

				std::ostringstream oss;
				for (ULONG i = 0; i < pCurr->PhysicalAddressLength; ++i)
				{
					if (i > 0) oss << ":";
					oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pCurr->PhysicalAddress[i]);
				}
				return oss.str();
			}
		}
		return "";
	}

	inline std::string sha256_hex(const std::string& input)
	{
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		std::string hash_hex;

		if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
			{
				CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0);
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
		return hash_hex;
	}

	inline std::string get_hwid_raw()
	{
		std::string uuid;
		std::string mb_serial;
		get_smbios_data(uuid, mb_serial);

		std::string disk_serial = get_physical_drive_serial();
		std::string cpu_info = get_cpuid_info();
		std::string mac = get_physical_mac();
		std::string machine_guid = get_machine_guid();

		std::ostringstream raw;
		raw << "UUID:" << (uuid.empty() ? "N/A" : uuid) << "|"
			<< "MB:" << (mb_serial.empty() ? "N/A" : mb_serial) << "|"
			<< "DISK:" << (disk_serial.empty() ? "N/A" : disk_serial) << "|"
			<< "CPU:" << (cpu_info.empty() ? "N/A" : cpu_info) << "|"
			<< "MAC:" << (mac.empty() ? "N/A" : mac) << "|"
			<< "GUID:" << (machine_guid.empty() ? "N/A" : machine_guid);

		return raw.str();
	}

	inline std::string get_hwid()
	{
		std::string raw_str = get_hwid_raw();
		std::string hash = sha256_hex(raw_str);
		return hash.empty() ? raw_str : hash;
	}
}

