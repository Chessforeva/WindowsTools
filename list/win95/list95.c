/*

 DIR *.* /S > DLIST.TXT
  for Windows 95 long file names
  8-bit chars only

  in simplest old C style for TinyC compiler
 
 Compile:
 tcc.exe list95.c -IINCLUDE -o list95.exe
 
 Chessforeva, 2025
 
*/

#define _WIN32_WINNT 0x0501 // Ensure compatibility with Windows XP
#include <windows.h>
#include <stdio.h>

char buffer_pth[200000];
char buffer_poi[800000];
char buffer_sub[3000];
char *pth = buffer_pth;
char *poi = buffer_poi;
char *sub = buffer_sub;
FILE *file;
char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char* dirtag[] = {"<dir>","     "};
char attrib[20];  
char formatted_time[30];
char formshort_time[30];

void get_attributes( WIN32_FIND_DATA *find_data ) {
    DWORD w = (DWORD)find_data->dwFileAttributes;
    for(int i=0; i<5; i++) attrib[i]=' ';
    if( w & FILE_ATTRIBUTE_DIRECTORY ) attrib[0] = 'D';
    if( w & FILE_ATTRIBUTE_HIDDEN ) attrib[1] = 'H';
    attrib[3]=0;
}

void list_file_folder( WIN32_FIND_DATA *find_data ) {
    //char *dir = (attrib[0]=='D') ? dirtag[0] : dirtag[1];
    SYSTEMTIME st;
    FileTimeToSystemTime( &find_data->ftLastWriteTime, &st); // Now using FileTimeToSystemTime    
    //sprintf(formatted_time, "%s %02d %04d %02d:%02d", months[st.wMonth-1], st.wDay, st.wYear, st.wHour, st.wMinute);
    sprintf(formshort_time, "%s %02d %04d", months[st.wMonth-1], st.wDay, st.wYear);
    DWORD size = find_data->nFileSizeLow;    // under 4GB
    //fprintf(file, "%s %s %12lu %s %s\n", dir, attrib, size, formatted_time, find_data->cFileName);
    fprintf(file, "%c%c %12lu %s %s\n", attrib[0], attrib[1], size, formshort_time, find_data->cFileName);
}

int save_file_folder( WIN32_FIND_DATA *find_data ) {
    char *s = (char *)(find_data->cFileName);
    if( strcmp( s, "." ) == 0 || strcmp( s, ".." ) == 0 ) {
        return 0;
    } 
    while(1) {
        *poi = *s;
        if( *poi == '\0' ) break;
        poi++; s++;
    }
    *(++poi) = '\0';
    return 1;
}
        
void pth_skipend() {
    while( *pth != '\0' ) pth++;
    *(++pth) = '\0';    // skip 0, set end 0
}

void sub_addtoend( char *s ) {
    while(1) {
        *sub = *s;
        if( *sub == '\0' ) break;
        sub++; s++;
    }
}

void dirlist( int depth ) {
    
    WIN32_FIND_DATA find_data;
    HANDLE hFind;

    char *c_pth = pth;
    char *c_poi = poi;
    char *p = poi;
    
    int cnt = 0;
    GetCurrentDirectory(3000, pth);
    
    // current folder print
    fprintf( file, "\n%s\n\n", pth );

    pth_skipend();
    
    hFind = FindFirstFile("*", &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(file, "Error reading folder.\n");
        return;
    }
    do {
        get_attributes( &find_data );
        list_file_folder( &find_data );      
        if(attrib[0]=='D') {
            if( save_file_folder( &find_data ) ) cnt++;
        }
    } while (FindNextFile(hFind, &find_data));
    
        // scan subfolders
    while(cnt--) {
        sub = buffer_sub;
        *sub = '\0';
        sub_addtoend( c_pth );
        if(*(--sub) != '\\') {
            *(++sub) = '\\';
            *(++sub) = '\0';
            }
        else sub++;
        sub_addtoend( p );
        if(depth == 0) printf("%s\n", p);
        while( *p != '\0' ) p++;
        p++; // skip 0
        if(SetCurrentDirectory( buffer_sub )) {
            dirlist( depth+1 );
        }
        else {
            fprintf(file, "Error, use list_w16.exe tool: %s \n", buffer_sub);
        }
        SetCurrentDirectory( c_pth );
    }
    pth = c_pth;
    poi = c_poi;

}
    
int main(int argc, char **argv) {
    char *output = "DLIST.TXT";
    *pth = '\0';
    *poi = '\0';
    file = fopen(output,"wt");
    if(!file) {
        printf("File creation error. Exiting.\n");
        return 1;
        }
    printf("Scanning directories and writing %s....\n", output);   
    dirlist(0);
    printf("ok\n"); 
    fclose(file);
}
