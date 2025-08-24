
#include <header.h>

#include <Strsafe.h>
#include <string>

//
//
//
std::string GetLastErrorText()
{
	LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&lpMsgBuf),
        0,
        NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(
        LMEM_ZEROINIT, 
        (lstrlen(static_cast<LPCTSTR>(lpMsgBuf))
        + 40) * sizeof(TCHAR)
        ); 

    StringCchPrintf(static_cast<LPTSTR>(lpDisplayBuf), 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("Failed with error %d: %s"), 
        dw,
        lpMsgBuf); 

    std::string message = std::string(static_cast<char*>(lpDisplayBuf));

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

    return message;
}
