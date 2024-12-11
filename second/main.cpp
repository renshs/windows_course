#include "windows.h"
#include "stdio.h"

BOOL LoadPeFile(LPCWSTR FilePath, PUCHAR* ppImageBase)
{
    HANDLE hFile = CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        printf("ERROR: LoadPeFile: CreateFile fails with %d error \n", GetLastError());
        return false;
    }

    HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE_NO_EXECUTE, 0, 0, NULL);
    if (NULL == hFileMapping) {
        printf("ERROR: LoadPeFile: CreateFileMapping fails with %d error \n", GetLastError());
        CloseHandle(hFile);
        return false;
    }

    LPVOID p = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (NULL == p) {
        printf("ERROR: LoadPeFile: MapViewOfFile fails with %d error \n", GetLastError());
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return false;
    }

    *ppImageBase = (PUCHAR)p;
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
    return true;
}

#define TO_PSTRUCT(TYPE, offset) (TYPE)(pImageBase + (offset)) // Convert RVA to structure pointer
#define VAR_OF_PSTRUCT(var, TYPE, offset) TYPE var = TO_PSTRUCT(TYPE, offset)

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 2) {
        printf("Usage: PEAnalyzer <PeFilePath>\n");
        return -1;
    }

    LPCWSTR g_FilePath = argv[1];
    PUCHAR pImageBase = nullptr;
    if (!LoadPeFile(g_FilePath, &pImageBase)) return -1;

    printf("MS-DOS Signature: %c%c \n", pImageBase[0], pImageBase[1]);
    if (pImageBase[0] != 'M' || pImageBase[1] != 'Z') {
        printf("ERROR: Not a valid PE file\n");
        return -1;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pImageBase;
    VAR_OF_PSTRUCT(pTempPeHeader, PIMAGE_NT_HEADERS, pDosHeader->e_lfanew); // Offset to PE Header

    PUCHAR p = (PUCHAR)(&pTempPeHeader->Signature);
    if (p[0] != 'P' || p[1] != 'E') {
        printf("ERROR: Invalid PE signature\n");
        return -1;
    }

    printf("PE Signature: %c%c %x%x \n", p[0], p[1], p[2], p[3]);

    WORD nSections = pTempPeHeader->FileHeader.NumberOfSections;
    printf("PE Sections total: %d\n", nSections);

    PIMAGE_SECTION_HEADER pSectionHeader = nullptr;
    switch (pTempPeHeader->FileHeader.Machine) {
        case IMAGE_FILE_MACHINE_I386:
            printf("PE Architecture: x86\n");
            pSectionHeader = (PIMAGE_SECTION_HEADER)(((PUCHAR)pTempPeHeader) + sizeof(IMAGE_NT_HEADERS32));
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            printf("PE Architecture: x64\n");
            pSectionHeader = (PIMAGE_SECTION_HEADER)(((PUCHAR)pTempPeHeader) + sizeof(IMAGE_NT_HEADERS64));
            break;
        default:
            printf("PE Architecture: unknown\n");
            return -1;
    }

    CHAR nmSection[9];
    memset(nmSection, 0, sizeof(nmSection));
    for (int i = 0; i < nSections; i++) {
        memcpy(nmSection, pSectionHeader->Name, 8);
        printf("Section #%d: %s\n", i + 1, nmSection);
        pSectionHeader++;
    }

    UnmapViewOfFile(pImageBase);
    return 0;
}