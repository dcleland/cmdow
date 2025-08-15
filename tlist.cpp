#include "header.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#define TITLE_SIZE 64
#define PROCESS_SIZE MAX_PATH
#define INITIAL_SIZE 51200
#define EXTEND_SIZE 25600
#define REGKEY_PERF "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS "Counters"
#define PROCESS_COUNTER "process"
#define PROCESSID_COUNTER "id process"
#define UNKNOWN_TASK "Unknown"

static std::string toLower(const std::string &str)
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
static std::tuple<unsigned long, unsigned long> get_counter_ids(const LPBYTE buf, const DWORD dwSize)
{
    unsigned long process_id = 0;
    unsigned long id_process_id = 0;
    int ids_found = 0;

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
        if (toLower(name) == PROCESS_COUNTER)
        {
            process_id = id;
            ++ids_found;
            // Exit once both ids are found
            if (ids_found >= 2)
            {
                break;
            }
        }
        else if (toLower(name) == PROCESSID_COUNTER)
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
    DWORD dwSize;
    LPBYTE buf = NULL;
    CHAR szSubKey[1024];
    LPSTR p;
    LPSTR p2;
    PPERF_DATA_BLOCK pPerf;
    PPERF_OBJECT_TYPE pObj;
    PPERF_INSTANCE_DEFINITION pInst;
    PPERF_COUNTER_BLOCK pCounter;
    PPERF_COUNTER_DEFINITION pCounterDef;
    DWORD i;
    // DWORD                        dwProcessIdTitle;
    DWORD dwProcessIdCounter;
    CHAR szProcessName[MAX_PATH];
    DWORD dwNumTasks;

    dwSize = 20000;
    auto buf2 = std::vector<BYTE>(dwSize);

    // this returns "009" on my system
    LANGID lid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
    wsprintf(szSubKey, "%s\\%03x", REGKEY_PERF, lid);

    // Get handle to "khlm\sw\ms\winnt\cv\perflib\009"
    DWORD rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKeyNames);
    if (rc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // get the buffer size for the counter value
    rc = RegQueryValueEx(hKeyNames, REGSUBKEY_COUNTERS, NULL, &dwType, NULL, &dwSize);

    if (rc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // allocate the counter names buffer
    buf = (LPBYTE)malloc(dwSize);

    if (buf == NULL)
    {
        goto exit;
    }
    memset(buf, 0, dwSize);

    // read the counter names from the registry
    rc = RegQueryValueEx(hKeyNames, REGSUBKEY_COUNTERS, NULL, &dwType, buf, &dwSize);

    if (rc != ERROR_SUCCESS)
    {
        goto exit;
    }

    p = reinterpret_cast<char *>(buf);

    // dwProcessIdTitle = 0; // added by me as dwProcessIdTitle may be used without init

    auto [process_id, id_process_id] = get_counter_ids(buf, dwSize);
    if (process_id == 0 || id_process_id == 0)
    {
        (void)fprintf(stderr, "Failed to get both ids\n");
        exit(2);
    }

    dwSize = INITIAL_SIZE;
    buf = (LPBYTE)malloc(dwSize);
    if (buf == NULL)
    {
        goto exit;
    }

    memset(buf, 0, dwSize);
    while (TRUE)
    {

        rc = RegQueryValueEx(HKEY_PERFORMANCE_DATA, std::to_string(process_id).c_str(), NULL, &dwType, buf, &dwSize);
        pPerf = (PPERF_DATA_BLOCK)buf;
        if ((rc == ERROR_SUCCESS) && (dwSize > 0) && (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' && (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F')
        {
            break;
        }
        if (rc == ERROR_MORE_DATA)
        {
            dwSize += EXTEND_SIZE;
            buf = (LPBYTE)realloc(buf, dwSize);
            memset(buf, 0, dwSize);
        }
        else
        {
            goto exit;
        }
    }

    pObj = reinterpret_cast<PPERF_OBJECT_TYPE>(reinterpret_cast<char *>(pPerf) + pPerf->HeaderLength);
    pCounterDef = reinterpret_cast<PPERF_COUNTER_DEFINITION>(reinterpret_cast<char *>(pObj) + pObj->HeaderLength);
    for (DWORD i = 0; i < pObj->NumCounters; i++)
    {
        if (pCounterDef->CounterNameTitleIndex == id_process_id)
        {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    dwNumTasks = (DWORD)pObj->NumInstances;

    pInst = (PPERF_INSTANCE_DEFINITION)((char *)pObj + pObj->DefinitionLength);

    for (LONG ndx = 0; ndx < dwNumTasks; ndx++)
    {
        p = (LPSTR)(reinterpret_cast<char *>(pInst) + pInst->NameOffset);
        rc = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)p, -1, szProcessName, sizeof(szProcessName), NULL, NULL);

        if (!rc)
        {
            lstrcpy(tlist->ProcessName, UNKNOWN_TASK);
        }

        // load process name into tlist
        lstrcpy(tlist->ProcessName, szProcessName);

        // load Pid into tlist
        pCounter = reinterpret_cast<PPERF_COUNTER_BLOCK>(reinterpret_cast<char *>(pInst) + pInst->ByteLength);
        tlist->dwProcessId = *(reinterpret_cast<LPDWORD>(reinterpret_cast<char *>(pCounter) + dwProcessIdCounter));
        pInst = reinterpret_cast<PPERF_INSTANCE_DEFINITION>(reinterpret_cast<char *>(pCounter) + pCounter->ByteLength);

        // create a new tlist record
        tlist->next = static_cast<struct TLIST *>(HeapAlloc(GetProcessHeap(), 0, sizeof(struct TLIST)));
        if (!(tlist->next))
            return 1;

        tlist = tlist->next; // point to the new record
        tlist->next = NULL;  // in case this is last record, null serves as a flag
                             // note, last record of tlist will be undefined except next which is NULL
    }
    return 0;

exit:
    if (buf)
        free(buf);
    RegCloseKey(hKeyNames);
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    return 1;
}

char *GetImageName(long pid)
{
    static int init;
    static struct TLIST tlist;
    static char Unknown[] = "Unknown";

    if (!init++)
    {
        tlist.next = NULL;
        if (GetTaskList(&tlist))
            return NULL;
        // build list of tasks
    }

    struct TLIST *t = &tlist;
    while (t->next)
    {
        if (t->dwProcessId == (DWORD)pid)
        {
            return t->ProcessName;
        }
        t = t->next;
    }
    return Unknown;
}
