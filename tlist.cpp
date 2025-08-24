#include "header.h"
#include "registry_wrappers.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <fmt/core.h>

enum
{
	TITLE_SIZE = 64
};

#define PROCESS_SIZE MAX_PATH

enum
{
	INITIAL_SIZE = 51200,
	EXTEND_SIZE = 25600
};

// constexpr auto REGKEY_PERF = R"(software\microsoft\windows nt\currentversion\perflib)";
#define REGKEY_PERF "software\\microsoft\\windows nt\\currentversion\\perflib"

constexpr auto REGSUBKEY_COUNTERS = "Counters";

#define PROCESS_COUNTER "process"
#define PROCESSID_COUNTER "id process"
#define UNKNOWN_TASK "Unknown"

static std::string to_lower(const std::string &str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

struct TLIST
{
    DWORD dwProcessId;
    CHAR ProcessName[PROCESS_SIZE];
    struct TLIST *next;
};

/// <summary>
/// Extract the ids of the "process" and "id process"
/// </summary>
/// <param name="buf">Buffer to be searched</param>
/// <param name="dwSize">Buffer length</param>
/// <returns>Tuple of ids (process and id process) or -1, -1</returns>
static std::tuple<unsigned long, unsigned long> get_counter_ids(
	const LPBYTE buf,
	const DWORD dwSize)
{
    unsigned long process_id = 0;
    unsigned long id_process_id = 0;
    int ids_found = 0;

	// Basic check that the buffer looks correct.
	assert(isdigit(buf[0]));

    auto p = buf;
    bool bContinue = true;
    while (p < p + dwSize && bContinue)
    {
        char *endPtr;

        // Get id
        auto id = strtoul(reinterpret_cast<const char *>(p), &endPtr, 10);
        if (errno == ERANGE)
        {
            printf("Error converting\n");
            exit(1); // NOLINT(concurrency-mt-unsafe)
        }
        if (reinterpret_cast<char *>(p) == endPtr)
        {
            // Break if end of data reached
            break;
        }

        p = p + strlen(reinterpret_cast<const char *>(p)) + 1;
        auto name = std::string(reinterpret_cast<const char *>(p));
        if (to_lower(name) == PROCESS_COUNTER)
        {
            process_id = id;
            ++ids_found;
            // Exit once both ids are found
            if (ids_found >= 2)
            {
                break;
            }
        }
        else if (to_lower(name) == PROCESSID_COUNTER)
        {
            id_process_id = id;
            ++ids_found;
            // Exit once both ids are found
            if (ids_found >= 2)
            {
                break;
            }
        }
        p = p + name.length() + 1;
    }

    return std::make_tuple(process_id, id_process_id);
}

int GetTaskList(struct TLIST *tlist)
{
	HKEY hKeyNames;
	DWORD dwType;
	LPBYTE buf = nullptr;
	CHAR szSubKey[1024];
	LPSTR p;
	LPSTR p2;
	PPERF_DATA_BLOCK pPerf;
	PPERF_OBJECT_TYPE pObj;
	PPERF_INSTANCE_DEFINITION pInst;
	PPERF_COUNTER_BLOCK pCounter;
	PPERF_COUNTER_DEFINITION pCounterDef;
	// DWORD                        dwProcessIdTitle;
	DWORD dwProcessIdCounter;
	CHAR szProcessName[MAX_PATH];
	DWORD dwNumTasks;

	DWORD dwSize = 20000;
	auto buf2 = std::vector<BYTE>(dwSize);

	// this returns "009" on my system
	LANGID lid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
	// wsprintf(szSubKey, "%s\\%03x", REGKEY_PERF, lid);

	auto stSubKey = fmt::format(
		"{0}\\{1:03x}",
		REGKEY_PERF, 
		lid);

	try
	{
		RegistryKey regKey(HKEY_LOCAL_MACHINE,
			stSubKey,
			KEY_READ);
		HKEY hKey = regKey.get();

		RegistryKeyValue regKeyValue(regKey,
			REGSUBKEY_COUNTERS
			);

		auto [process_id, id_process_id] = get_counter_ids(
				const_cast<LPBYTE>(regKeyValue.data()), regKeyValue.size());

		if (process_id == 0 || id_process_id == 0)
		{
			(void)fprintf(stderr, "Failed to get both ids\n");
			exit(2);  // NOLINT(concurrency-mt-unsafe)
		}

		RegistryKeyValue regPerfKeyValue(HKEY_PERFORMANCE_DATA,
			std::to_string(process_id)
		);
		pPerf = PPERF_DATA_BLOCK(regPerfKeyValue.data());

		if (regPerfKeyValue.size() > 0 &&
			pPerf->Signature[0] == static_cast<WCHAR>('P') &&
			pPerf->Signature[1] == static_cast<WCHAR>('E') &&
			pPerf->Signature[2] == static_cast<WCHAR>('R') &&
			pPerf->Signature[3] == static_cast<WCHAR>('F'))
		{
			// OK. Do nothing
		}
		else
		{
			throw std::runtime_error("Failed to read Performance counters");
		}

		pObj = reinterpret_cast<PPERF_OBJECT_TYPE>(
			reinterpret_cast<char*>(pPerf) + pPerf->HeaderLength);
		pCounterDef = reinterpret_cast<PPERF_COUNTER_DEFINITION>(
			reinterpret_cast<char*>(pObj) + pObj->HeaderLength);
		for (DWORD i = 0; i < pObj->NumCounters; i++)
		{
			if (pCounterDef->CounterNameTitleIndex == id_process_id)
			{
				dwProcessIdCounter = pCounterDef->CounterOffset;
				break;
			}
			pCounterDef++;
		}

		dwNumTasks = static_cast<DWORD>(pObj->NumInstances);
		pInst = reinterpret_cast<PPERF_INSTANCE_DEFINITION>(
			reinterpret_cast<char*>(pObj) + pObj->DefinitionLength);
		
		for (auto ndx = 0u; ndx < dwNumTasks; ndx++)
		{
			p = reinterpret_cast<char*>(pInst) + pInst->NameOffset;
			auto rc = WideCharToMultiByte(
				CP_ACP, 
				0, 
				reinterpret_cast<LPCWSTR>(p), 
				-1, 
				szProcessName, 
				sizeof(szProcessName),
				nullptr,
			nullptr);

			if (!rc)
			{
				lstrcpy(tlist->ProcessName, UNKNOWN_TASK);
			}

			// load process name into tlist
			lstrcpy(tlist->ProcessName, szProcessName);

			// load Pid into tlist
			pCounter = reinterpret_cast<PPERF_COUNTER_BLOCK>(
				reinterpret_cast<char*>(pInst) + pInst->ByteLength);
			tlist->dwProcessId = *(
				reinterpret_cast<LPDWORD>(
					reinterpret_cast<char*>(pCounter) + dwProcessIdCounter));
			pInst = reinterpret_cast<PPERF_INSTANCE_DEFINITION>(
				reinterpret_cast<char*>(pCounter) + pCounter->ByteLength);

			// create a new tlist record
			tlist->next = static_cast<struct TLIST*>(
				HeapAlloc(GetProcessHeap(), 0, sizeof(struct TLIST)));
			if (!(tlist->next))
			{
				return 1;
			}

			tlist = tlist->next; // point to the new record
			tlist->next = nullptr;  // in case this is last record, null serves as a flag
			// note, last record of tlist will be undefined except next which is NULL
		}
		return 0;
	}
	catch (const std::exception& e) {
		// Handle error
		printf("Error: %s\n", e.what());
	}
	return 0;
}

char *GetImageName(long pid)
{
    static int init;
    static struct TLIST tlist;
    static char Unknown[] = "Unknown";

    if (!init++)
    {
        tlist.next = nullptr;
		if (GetTaskList(&tlist))
		{
			return nullptr;
		}
    }

    struct TLIST *t = &tlist;
    while (t->next)
    {
        if (t->dwProcessId == static_cast<DWORD>(pid))
        {
            return t->ProcessName;
        }
        t = t->next;
    }
    return Unknown;
}
