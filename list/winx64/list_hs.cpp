/*

 DIR *.* /s /a > DLIST.TXT
  for Windows x64 as Win10,Win11,etc...
    Modernized style, no compiler warnings.

 2-byte encoded filenames
 
 MS Visual Studio 2022 C++ compiled

 Chessforeva, 2025

*/

#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

FILE* file;
const wchar_t* months[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
char attrib[20];

void get_attributes(WIN32_FIND_DATAW* find_data) {
    DWORD w = (DWORD)find_data->dwFileAttributes;
    for (int i = 0; i < 5; i++) attrib[i] = ' ';
    if (w & FILE_ATTRIBUTE_DIRECTORY) attrib[0] = 'D';
    if (w & FILE_ATTRIBUTE_HIDDEN) attrib[1] = 'H';
    if (w & FILE_ATTRIBUTE_SYSTEM) attrib[2] = 'S';
    if (w & FILE_ATTRIBUTE_REPARSE_POINT) attrib[3] = 'J';
    attrib[4] = 0;
}

void list_file_folder(WIN32_FIND_DATAW* find_data) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&find_data->ftLastWriteTime, &st); // Now using FileTimeToSystemTime   

    ULARGE_INTEGER fileSize;
    fileSize.HighPart = find_data->nFileSizeHigh;
    fileSize.LowPart = find_data->nFileSizeLow;
    unsigned long long size = fileSize.QuadPart;

    fwprintf(file, L"%c%c%c%c %16llu %ls %02d %04d %ls\n",
        attrib[0], attrib[1], attrib[2], attrib[3], size,
        months[st.wMonth-1], st.wDay, st.wYear, find_data->cFileName);

}

// really can not find a good conversion function
void simplecopy(wchar_t *to, char *from) {
    while (1) {
        *to = *from;
        if (*from == '\0') break;
        to++; from++;
    }
    *to = L'\0';
}

void findFilesInSubfolders(const std::wstring& basePath, int depth) {
    std::vector<std::wstring> foundFiles;

    WIN32_FIND_DATAW findData;

     std::wstring base = basePath;
    if (*base.rbegin() != '\\') base += '\\';
    
    std::wstring searchPath = base + L"*"; // Search all files and folders in the base path

    std::vector<std::wstring> foundEntities;

    // current folder print
    fwprintf(file, L"\n%ls\n\n", basePath.c_str());

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            get_attributes(&findData);
            list_file_folder(&findData);

            if (attrib[0]=='D') {
                // It's a directory.  Recurse if it's not "." or ".."
                if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                    std::wstring subfolderPath = base + findData.cFileName + L"\\";
                    foundEntities.push_back(subfolderPath);     // save
                }
            }
        } while (FindNextFileW(hFind, &findData) != 0);

        for (const auto& foldPath : foundEntities) {
            if (depth == 0) {
                wprintf(L"%ls\n", foldPath.c_str());
            }
            findFilesInSubfolders(foldPath, depth+1);
        }

        FindClose(hFind);
    }
    else {
        fwprintf(file, L"Error reading folder.\n");
    }
}


int main(int argc, char** argv) {

    const wchar_t* output = L"DLIST.TXT";
    wchar_t pth[3000];

    if (argc > 1) {
        simplecopy(pth, argv[1]);
    }
    else {
        GetCurrentDirectoryW(3000, pth);
    }

    errno_t err = _wfopen_s(&file, output, L"w+, ccs=UTF-8");
    if (err) {
        wprintf(L"File creation error. Exiting.\n");
        return 1;
    }
    wprintf(L"Scanning directories and writing %ls....\n", output);
    findFilesInSubfolders(pth, 0);
    wprintf(L"ok\n");
    if(file) fclose(file);

    return 0;
}
