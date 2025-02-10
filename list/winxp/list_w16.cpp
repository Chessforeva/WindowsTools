/*

 DIR *.* /s /a:h > DLIST.TXT

  for Windows XP long file names
  
 2-byte encoded filenames, not 8-bit
 
 MS Visual C++ 2010 version, it is Microsoft
 
 Chessforeva, 2025
 
*/
#include "stdafx.h"

#define _WIN32_WINNT 0x0501 // Ensure compatibility with Windows XP
#include <windows.h>
#include <stdio.h>

wchar_t  buffer_pth[200000];
wchar_t  buffer_poi[800000];
wchar_t  buffer_sub[3000];
wchar_t  *pth = buffer_pth;
wchar_t  *poi = buffer_poi;
wchar_t  *sub = buffer_sub;
FILE *file;
wchar_t* months[] = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
wchar_t* dirtag[] = {L"<dir>",L"     "};
char attrib[20];  
char formatted_time[30];
char formshort_time[30];
wchar_t formshort_timeW[30];

void get_attributes( WIN32_FIND_DATAW *find_data ) {
    DWORD w = (DWORD)find_data->dwFileAttributes;
    for(int i=0; i<5; i++) attrib[i]=' ';
    if( w & FILE_ATTRIBUTE_DIRECTORY ) attrib[0] = 'D';
    if( w & FILE_ATTRIBUTE_HIDDEN ) attrib[1] = 'H';
    attrib[3]=0;
}

void list_file_folder( WIN32_FIND_DATAW *find_data ) {
    //wchar_t *dir = (attrib[0]=='D') ? dirtag[0] : dirtag[1];
    SYSTEMTIME st;
    FileTimeToSystemTime( &find_data->ftLastWriteTime, &st); // Now using FileTimeToSystemTime    
    DWORD size = find_data->nFileSizeLow;    // under 4GB
    fwprintf(file, L"%c %12lu %ls %02d %04d %ls\n", attrib[0], size, months[st.wMonth-1], st.wDay, st.wYear, find_data->cFileName);
}

int save_file_folder( WIN32_FIND_DATAW *find_data ) {
    wchar_t *s = (wchar_t *)(find_data->cFileName);
    if( wcscmp( s, L"." ) == 0 ||wcscmp( s, L".." ) == 0 ) {
        return 0;
    } 
    while(1) {
        *poi = *s;
        if( *poi == L'\0' ) break;
        poi++; s++;
    }
    *(++poi) = L'\0';
    return 1;
}
        
void pth_skipend() {
    while( *pth != L'\0' ) pth++;
    *(++pth) = L'\0';    // skip 0, set end 0
}

void sub_addtoend( wchar_t *s ) {
    while(1) {
        *sub = *s;
        if( *sub == L'\0' ) break;
        sub++; s++;
    }
}

void dirlist( int depth ) {
    
    WIN32_FIND_DATAW find_data;
    HANDLE hFind;

    wchar_t *c_pth = pth;
    wchar_t *c_poi = poi;
    wchar_t *p = poi;
    
    int cnt = 0;
    GetCurrentDirectoryW(3000, pth);
    
    // current folder print
    fwprintf( file, L"\n%s\n\n", pth );

    pth_skipend();
    
    hFind = FindFirstFileW(L"*", &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        fwprintf(file, L"Error reading folder.\n");
        return;
    }
    do {
        get_attributes( &find_data );
        list_file_folder( &find_data );      
        if(attrib[0]=='D') {
            if( save_file_folder( &find_data ) ) cnt++;
        }
    } while (FindNextFileW(hFind, &find_data));
    
        // scan subfolders
    while(cnt--) {
        sub = buffer_sub;
        *sub = L'\0';
        sub_addtoend( c_pth );
        if(*(--sub) != L'\\') {
            *(++sub) = L'\\';
            *(++sub) = L'\0';
            }
        else sub++;
        sub_addtoend( p );
        if(depth == 0) wprintf(L"%s\n", p);
        while( *p != L'\0' ) p++;
        p++; // skip 0
        if(SetCurrentDirectoryW( buffer_sub )) {
            dirlist( depth+1 );
        }
        else {
            fwprintf(file, L"Error, can not set: %s\n", buffer_sub);
        }
        SetCurrentDirectoryW( c_pth );
    }
    pth = c_pth;
    poi = c_poi;

}
    
int main(int argc, char **argv) {
    wchar_t *output = L"DLIST.TXT";
    *pth = L'\0';
    *poi = L'\0';
    errno_t err = _wfopen_s(&file, output, L"w+, ccs=UTF-8");
    if(err) {
        printf("File creation error. Exiting.\n");
        return 1;
        }
    wprintf(L"Scanning directories and writing %s....\n", output);   
    dirlist(0);
    printf("ok\n"); 
    fclose(file);
}
