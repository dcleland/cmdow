#include <algorithm>

#include "header.h"

#include <cassert>
#include <string>
#include <algorithm>

#define PROCESS_COUNTER "process"
#define PROCESSID_COUNTER "id process"
#define UNKNOWN_TASK "Unknown"

static std::string to_lower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}


/// <summary>
/// Extract the ids of the "process" and "id process"
/// </summary>
/// <param name="buf">Buffer to be searched</param>
/// <param name="dwSize">Buffer length</param>
/// <returns>Tuple of ids (process and id process) or -1, -1</returns>
std::tuple<unsigned long, unsigned long> get_counter_ids(
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
        char* endPtr;

        // Get id
        auto id = strtoul(reinterpret_cast<const char*>(p), &endPtr, 10);
        if (errno == ERANGE)
        {
            printf("Error converting\n");
            exit(1); // NOLINT(concurrency-mt-unsafe)
        }
        if (reinterpret_cast<char*>(p) == endPtr)
        {
            // Break if end of data reached
            break;
        }

        p = p + strlen(reinterpret_cast<const char*>(p)) + 1;
        auto name = std::string(reinterpret_cast<const char*>(p));
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
