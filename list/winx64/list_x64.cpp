/*

 DIR *.* /S > DLIST.TXT
  for Windows x64 as Win10,Win11,etc...

 2-byte encoded filenames
 800 Mb memory allocations

 MS Visual Studio 2022 C++ compiled

 Chessforeva, 2025

*/

#include <windows.h>
#include <stdio.h>

#define MALLOC_PATHS (20*1000*1000)
#define MALLOC_POINTR (800*1000*1000)
#define MALLOC_SUBFLDR (1*1000*1000)

wchar_t* buffer_pth, * pth;
wchar_t* buffer_poi, * poi;
wchar_t* buffer_sub, * sub;
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
        attrib[0], attrib[1], attrib[2], attrib[3],
        fileSize, months[st.wMonth - 1], st.wDay, st.wYear, find_data->cFileName);
}

int save_file_folder(WIN32_FIND_DATAW* find_data) {
    wchar_t* s = (wchar_t*)(find_data->cFileName);
    if (wcscmp(s, L".") == 0 || wcscmp(s, L"..") == 0) {
        return 0;
    }
    while (1) {
        *poi = *s;
        if (*poi == L'\0') break;
        poi++; s++;
    }
    *(++poi) = L'\0';
    return 1;
}

void pth_skipend() {
    while (*pth != L'\0') pth++;
    *(++pth) = L'\0';    // skip 0, set end 0
}

void sub_addtoend(wchar_t* s) {
    while (1) {
        *sub = *s;
        if (*sub == L'\0') break;
        sub++; s++;
    }
}

void dirlist(int depth) {

    WIN32_FIND_DATAW find_data;
    HANDLE hFind;

    wchar_t* c_pth = pth;
    wchar_t* c_poi = poi;
    wchar_t* p = poi;

    int cnt = 0;
    GetCurrentDirectoryW(3000, pth);

    // current folder print
    fwprintf(file, L"\n%s\n\n", pth);

    pth_skipend();

    hFind = FindFirstFileW(L"*", &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        fwprintf(file, L"Error reading folder.\n");
        return;
    }
    do {
        get_attributes(&find_data);
        list_file_folder(&find_data);
        if (attrib[0] == 'D') {
            if (save_file_folder(&find_data)) cnt++;
        }
    } while (FindNextFileW(hFind, &find_data));

    // scan subfolders
    while (cnt--) {
        sub = buffer_sub;
        *sub = L'\0';
        sub_addtoend(c_pth);
        if (*(--sub) != L'\\') {
            *(++sub) = L'\\';
            *(++sub) = L'\0';
        }
        else sub++;
        sub_addtoend(p);
        if (depth == 0) wprintf(L"%s\n", p);
        while (*p != L'\0') p++;
        p++; // skip 0
        if (SetCurrentDirectoryW(buffer_sub)) {
            dirlist(depth + 1);
        }
        else {
            fwprintf(file, L"Error, can not set: %s\n", buffer_sub);
        }
        SetCurrentDirectoryW(c_pth);
    }
    pth = c_pth;
    poi = c_poi;

}

int main(int argc, char** argv) {
    const wchar_t* output = L"DLIST.TXT";

    buffer_pth = (wchar_t*)malloc(sizeof(wchar_t) * MALLOC_PATHS);
    buffer_poi = (wchar_t*)malloc(sizeof(wchar_t) * MALLOC_POINTR);
    buffer_sub = (wchar_t*)malloc(sizeof(wchar_t) * MALLOC_SUBFLDR);
    pth = buffer_pth; *pth = L'\0';
    poi = buffer_poi; *poi = L'\0';
    sub = buffer_sub;
    errno_t err = _wfopen_s(&file, output, L"w+, ccs=UTF-8");
    if (err) {
        printf("File creation error. Exiting.\n");
        return 1;
    }
    wprintf(L"Scanning directories and writing %s....\n", output);
    dirlist(0);
    printf("ok\n");
    if (file) fclose(file);
    free(buffer_sub);
    free(buffer_poi);
    free(buffer_pth);
}
