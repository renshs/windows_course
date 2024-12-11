#include <iostream>
#include <windows.h>
#include <vector>
#include <string>

void PrintPEInfo(const std::string& file_path) {
    HANDLE file = CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Failed to open the file." << std::endl;
        return;
    }

    HANDLE file_mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!file_mapping) {
        std::cerr << "Error: Failed to create file mapping." << std::endl;
        CloseHandle(file);
        return;
    }

    LPVOID file_base = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
    if (!file_base) {
        std::cerr << "Error: Failed to map file to memory." << std::endl;
        CloseHandle(file_mapping);
        CloseHandle(file);
        return;
    }

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)file_base;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Error: Not a PE file." << std::endl;
        UnmapViewOfFile(file_base);
        CloseHandle(file_mapping);
        CloseHandle(file);
        return;
    }

    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)((BYTE*)file_base + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Error: Missing NT header." << std::endl;
        UnmapViewOfFile(file_base);
        CloseHandle(file_mapping);
        CloseHandle(file);
        return;
    }

    WORD machine = nt_headers->FileHeader.Machine;
    if (machine == IMAGE_FILE_MACHINE_I386) {
        std::cout << "Architecture: x86" << std::endl;
    } else if (machine == IMAGE_FILE_MACHINE_AMD64) {
        std::cout << "Architecture: x64" << std::endl;
    } else {
        std::cout << "Architecture: Unknown" << std::endl;
    }

    std::cout << "Sections:" << std::endl;
    PIMAGE_SECTION_HEADER section_header = IMAGE_FIRST_SECTION(nt_headers);
    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i) {
        std::cout << "  " << std::string((char*)section_header->Name, strnlen((char*)section_header->Name, IMAGE_SIZEOF_SHORT_NAME)) << std::endl;
        ++section_header;
    }

    UnmapViewOfFile(file_base);
    CloseHandle(file_mapping);
    CloseHandle(file);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path to PE file>" << std::endl;
        return 1;
    }

    PrintPEInfo(argv[1]);
    return 0;
}
