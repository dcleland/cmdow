#pragma once
#include <windows.h>

#include "get_last_error_text.hpp"

#include <string>
#include <stdexcept>
#include <vector>
#include <string_view>

class RegistryKey {
public:
    RegistryKey(HKEY hRootKey,
        std::string_view subKey,
        REGSAM accessRights)
	{
        LONG result = RegOpenKeyEx(
            hRootKey, 
            subKey.data(), 
            0, 
            accessRights,
            &hKey);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to open registry key. Error code: " + std::to_string(result));
        }
    }

    ~RegistryKey() {
        if (hKey)
        {
            RegCloseKey(hKey);
        }
    }

    HKEY get() const
    {
	    return hKey;
    }

private:
    HKEY hKey = nullptr;
};

class RegistryKeyValue {
public:
    RegistryKeyValue(const RegistryKey &key,
        const std::string& subKey)
    {
        keyName = subKey;
        LONG result = RegQueryValueEx(
            key.get(),
            subKey.c_str(),
            nullptr,
            &dwType,
            nullptr,
            &dwSize);
    	if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to query registry key. Error code: " + std::to_string(result));
        }

        buffer.resize(dwSize + 1);
        result = RegQueryValueEx(
            key.get(),
            subKey.c_str(),
            nullptr,
            &dwType,
            buffer.data(),
            &dwSize);
        if (result != ERROR_SUCCESS)
        {
            throw std::runtime_error("Failed to query registry key. Error code: " + std::to_string(result));
        }
    }

    /// <summary>
    /// Read from a pre-defined key type
    /// </summary>
    /// <param name="key"></param>
    /// <param name="subKey"></param>
    RegistryKeyValue(HKEY key,
        const std::string& subKey)
    {
        dwSize = INITIAL_SIZE;
        buffer.resize(dwSize);
        LONG result = RegQueryValueEx(
            key,
            subKey.c_str(),
            nullptr,
            &dwType,
            buffer.data(),
            &dwSize);
        bool bContinue = true;
        while (bContinue)
        {
            if (result == ERROR_SUCCESS && dwSize > 0)
            {
				break;
            }
            if (result == ERROR_MORE_DATA)
            {
                dwSize += EXTEND_SIZE;
                buffer.resize(dwSize);

                result = RegQueryValueEx(
                    key,
                    subKey.c_str(),
                    nullptr,
                    &dwType,
                    buffer.data(),
                    &dwSize);
            }
            else
            {
                auto msg = GetLastErrorText();
            	throw std::runtime_error(
                    "Failed to query registry key. Error code: "
                    + std::to_string(result) + "Msg: " + msg);
            }
        }
    }

    ~RegistryKeyValue() = default;

    const BYTE * data() const
    {
        return buffer.data();
    }

    DWORD size() const
    {
        return dwSize;
    }

private:
    enum
    {
        INITIAL_SIZE = 51200,
        EXTEND_SIZE = 25600
    };

    DWORD dwType = 0;
    DWORD dwSize = 0;
    std::string keyName;
    std::vector<BYTE> buffer;
};
