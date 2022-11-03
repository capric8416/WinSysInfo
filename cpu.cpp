// self
#include "cpu.h"

// c/c++
#include <intrin.h>
#include <thread>

// windows
#include <Windows.h>






// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}



QString CpuInfo::GetBrandName()
{
    int cpuInfo[4] = { -1 };
    char cpuBrand[0x40];

    memset(cpuBrand, 0, sizeof(cpuBrand));

    __cpuid(cpuInfo, 0x80000002);
    memcpy(cpuBrand, cpuInfo, sizeof(cpuInfo));

    __cpuid(cpuInfo, 0x80000003);
    memcpy(cpuBrand + 16, cpuInfo, sizeof(cpuInfo));

    __cpuid(cpuInfo, 0x80000004);
    memcpy(cpuBrand + 32, cpuInfo, sizeof(cpuInfo));

    return QString(cpuBrand);
}


bool CpuInfo::GetCores(unsigned int& sockets, unsigned int& physical, unsigned int& logicals)
{
    // Number of logical processors
    DWORD logicalProcessorCoreCount = 0;
    // Number of NUMA nodes
    DWORD numaNodeCount = 0;
    // Number of processor cores
    DWORD phisycalProcessorCoreCount = 0;
    // Number of processor L1/L2/L3 caches
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 0;

    typedef BOOL(WINAPI* LPFN_GetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
    LPFN_GetLogicalProcessorInformation GetLogicalProcessorInformation = (LPFN_GetLogicalProcessorInformation)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
    if (NULL == GetLogicalProcessorInformation)
    {
        return false;
    }

    BOOL done = FALSE;
    DWORD returnedLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    while (!done) {
        DWORD rc = GetLogicalProcessorInformation(buffer, &returnedLength);
        if (FALSE == rc) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer) {
                    free(buffer);
                }

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnedLength);
                if (NULL == buffer) {
                    // malloc fail
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            done = TRUE;
        }
    }

    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR cache;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnedLength)
    {
        switch (ptr->Relationship)
        {
        case RelationNumaNode:
        {
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
        }
        break;

        case RelationProcessorCore:
        {
            phisycalProcessorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCoreCount += CountSetBits(ptr->ProcessorMask);
        }
        break;

        case RelationCache:
        {
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
            cache = &ptr->Cache;
            if (cache->Level == 1)
            {
                processorL1CacheCount++;
            }
            else if (cache->Level == 2)
            {
                processorL2CacheCount++;
            }
            else if (cache->Level == 3)
            {
                processorL3CacheCount++;
            }
        }
        break;

        case RelationProcessorPackage:
        {
            // Logical processors share a physical package.
            processorPackageCount++;
        }
        break;

        default:
            // Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value
            break;
        }

        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    free(buffer);

    sockets = processorPackageCount;
    physical = phisycalProcessorCoreCount;
    logicals = logicalProcessorCoreCount;

    return true;
}