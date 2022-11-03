// self
#include "os.h"

// c/c++
#include <tuple>
#include <vector>

// windows
#include <Windows.h>



// To Workaround Win 10 problem for User Mode VerifyVersionInfo
bool VerifyWindowsVersionInfo(PRTL_OSVERSIONINFOEXW versionInfo, ULONG typeMask, ULONGLONG conditionMask)
{
    HMODULE hMod = nullptr;
    if ((TRUE != ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"ntdll.dll", &hMod)) || !hMod) {
        return false;
    }

    typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
    const auto RtlVerifyVersionInfo = reinterpret_cast<PFN_RtlVerifyVersionInfo>(::GetProcAddress(hMod, "RtlVerifyVersionInfo"));
    if (!RtlVerifyVersionInfo) {
        return false;
    }

    return RtlVerifyVersionInfo(versionInfo, typeMask, conditionMask) >= 0;
}


bool IsWindowsVersionOrGreater(int wMajorVersion, int wMinorVersion, int wBuildNumber, int wServicePackMajor)
{
    RTL_OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    osvi.dwMajorVersion = wMajorVersion;
    osvi.dwMinorVersion = wMinorVersion;
    osvi.dwBuildNumber = wBuildNumber;
    osvi.wServicePackMajor = static_cast<WORD>(wServicePackMajor);

    DWORDLONG dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

    return VerifyWindowsVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER | VER_SERVICEPACKMAJOR, dwlConditionMask);
}


QString OsInfo::GetName()
{
    // see https://msdn.microsoft.com/ru-ru/library/windows/desktop/ms724832(v=vs.85).aspx for details
    using OsMajorVersion = int;
    using OsMinorVersion = int;
    using OsBuildVersion = int;
    using OsServicePackMajorVersion = int;

    using VersionStrWithNumberPair =
        std::pair<std::wstring, std::tuple<OsMajorVersion, OsMinorVersion, OsBuildVersion, OsServicePackMajorVersion>>;


    const auto win11BuildNumber = 22000;

    const std::vector<VersionStrWithNumberPair> winVersionMapping = {
        {L"Windows 11", std::make_tuple(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), win11BuildNumber, 0)},
        {L"Windows 10", std::make_tuple(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 0, 0)},

        {L"Windows 8.1", std::make_tuple(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0, 0)},
        {L"Windows 8", std::make_tuple(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0, 0)},

        {L"Windows 7 Service Pack 1", std::make_tuple(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0, 1)},
        {L"Windows 7", std::make_tuple(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0, 0)},

        {L"Windows Vista Service Pack 2", std::make_tuple(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0, 2)},
        {L"Windows Vista Service Pack 1", std::make_tuple(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0, 1)},
        {L"Windows Vista", std::make_tuple(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0, 0)},

        {L"Windows XP Service Pack 3", std::make_tuple(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0, 3)},
        {L"Windows XP Service Pack 2", std::make_tuple(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0, 2)},
        {L"Windows XP Service Pack 1", std::make_tuple(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0, 1)},
        {L"Windows XP", std::make_tuple(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0, 0)} };

    const auto it = std::find_if(
        std::begin(winVersionMapping),
        std::end(winVersionMapping),
        [](const VersionStrWithNumberPair& el)
        {
            return IsWindowsVersionOrGreater(
                std::get<0>(el.second),
                std::get<1>(el.second),
                std::get<2>(el.second),
                std::get<3>(el.second)
            );
        }
    );

    if (it == std::end(winVersionMapping)) {
        return "Unknown windows";
    }
    return QString::fromStdWString(it->first);
}


QString OsInfo::GetVersions()
{
    const auto system = L"kernel32.dll";
    
    DWORD dummy = 0;
    const auto cbInfo = GetFileVersionInfoSizeEx(FILE_VER_GET_NEUTRAL, system, &dummy);

    std::vector<char> buffer(cbInfo);
    GetFileVersionInfoEx(FILE_VER_GET_NEUTRAL, system, dummy, buffer.size(), &buffer[0]);
    
    void* p = nullptr;
    UINT size = 0;
    VerQueryValue(buffer.data(), L"\\", &p, &size);
    
    auto pFixed = static_cast<const VS_FIXEDFILEINFO*>(p);
    return QString("%1.%2.%3.%4").arg(HIWORD(pFixed->dwFileVersionMS)).arg(LOWORD(pFixed->dwFileVersionMS)).arg(HIWORD(pFixed->dwFileVersionLS)).arg(LOWORD(pFixed->dwFileVersionLS));
}
