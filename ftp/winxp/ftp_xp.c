/*

 FTP server in simplest old C style for TinyC compiler for very old Windows PCs.

 ChatGPT made almost everything.
 Chessforeva, 2025 jan.

 Compile:
 tcc.exe ftp_xp.c -IINCLUDE -lws2_32 -o ftp_xp.exe
 
*/

#define _WIN32_WINNT 0x0501 // Ensure compatibility with Windows XP
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define Kb100 (100*1024)
#define BUFFER_SIZE Kb100

char buffer[Kb100];
char buff1[Kb100];
wchar_t utf16buffer[Kb100];   // 200Kbytes
char pathbuf[Kb100];
char list_buffer[Kb100*6];    // 600Kbytes
wchar_t filenameW[Kb100];     // 200Kbytes
char filenm[Kb100];
char buff2[Kb100];
char entry[Kb100];

char server_ip[256];
char client_ip[256];
char host_name[256];
int client_len;
struct sockaddr_in server_addr = {0}, client_addr = {0};
struct sockaddr_in active_addr = {0}, pasv_addr = {0};
FILE *file;
int use_w16 = 0;
WIN32_FIND_DATA find_data;
WIN32_FIND_DATAW find_dataW;
HANDLE hFind, hFindW;
int control_port = 21;
int data_port = 20;
int passive_mode = 1;
int transfer_mode = 0; // 0 = ASCII, 1 = Binary
int utf8 = 0;
WSADATA wsa_data;
SOCKET server_sock = INVALID_SOCKET, client_sock = INVALID_SOCKET;
SOCKET data_sock = INVALID_SOCKET, pasv_sock = INVALID_SOCKET;
char app_directory[MAX_PATH];
char working_directory[MAX_PATH] = "C:\\";

#define p_fLOGGEDIN 1
#define p_fREAD 2
#define p_fWRITE 4
#define p_fCHANGEROOT 8
#define p_fUSER 16
#define p_fPSW 32

int perm = 0;            // permission
int perm_saved;
char usr[20] = {0};
char psw[20] = {0};

int pm( int access ) {
    return (( (perm & access)==access ) ? 1 : 0 );
}

void get_server_ip(char *ip, size_t size) {   
    if (gethostname(host_name, sizeof(host_name)) == 0) {
        struct hostent *host_entry = gethostbyname(host_name);
        if (host_entry) {
            struct in_addr **addr_list = (struct in_addr **)host_entry->h_addr_list;
            if (addr_list[0]) {
                strncpy(ip, inet_ntoa(*addr_list[0]), size);
                return;
            }
        }
    }
    strncpy(ip, "127.0.0.1", size);
}

int convert_to_utf8( char *input ) {
    int input_length = strlen(input);
    unsigned int code_page = GetACP();
    int utf16_length = MultiByteToWideChar(code_page, 0, input, input_length, NULL, 0);
    if (utf16_length == 0) {
        printf("Error getting UTF-16 length: %d\n", GetLastError());
        return 1;
    }
    int result = MultiByteToWideChar(code_page, 0, input, input_length, utf16buffer, utf16_length);
    if (result == 0) {
        printf("Error converting to UTF-16: %d\n", GetLastError());
        return 1;
    }
    utf16buffer[utf16_length] = L'\0';
    int utf8_length = WideCharToMultiByte(CP_UTF8, 0, utf16buffer, utf16_length, NULL, 0, NULL, NULL);
    if (utf8_length == 0) {
        printf("Error getting UTF-8 length: %d\n", GetLastError());
        return 1;
    }
    result = WideCharToMultiByte(CP_UTF8, 0, utf16buffer, utf16_length, buffer, utf8_length, NULL, NULL);
    if (result == 0) {
        printf("Error converting to UTF-8: %d\n", GetLastError());
        return 1;
    }
    buffer[utf8_length] = '\0';
    printf("UTF8 converted\n");
    return 0;
}

int utf8_w16( char *input ) {
    int input_length = strlen(input);
    unsigned int code_page = GetACP();
    int utf16_length = MultiByteToWideChar(code_page, 0, input, input_length, NULL, 0);
    if (utf16_length == 0) return 1;
    int result = MultiByteToWideChar(code_page, 0, input, input_length, utf16buffer, utf16_length);
    if (result == 0) return 1;
    utf16buffer[utf16_length] = L'\0';
    return 0;
}

int w16utf8() {
    size_t utf16_length = wcslen(utf16buffer);
    utf16buffer[utf16_length] = L'\0';    
    int utf8_length = WideCharToMultiByte(CP_UTF8, 0, utf16buffer, utf16_length, NULL, 0, NULL, NULL);
    if (utf8_length == 0) return 1;
    int result = WideCharToMultiByte(CP_UTF8, 0, utf16buffer, utf16_length, buffer, utf8_length, NULL, NULL);
    if (result == 0) return 1;
    buffer[utf8_length] = '\0';
    return 0;
}

void send_response(char *message) {
    if(utf8) {
        convert_to_utf8( message );
        send(client_sock, buffer, strlen(buffer), 0);
        printf("%s\n",buffer);
    }
    else {
        send(client_sock, message, strlen(message), 0);
        printf("%s\n",message);
    }
}

SOCKET get_data_socket() {
    if (passive_mode) {
        struct sockaddr_in client;
        int client_len = sizeof(client);
        return accept(pasv_sock, (struct sockaddr*)&client, &client_len);
    }
    return data_sock;
}

void removecharat(char *s) {
    char *T = s, *Q = s;
    T++;
    while(*Q != 0) {
        *Q = *T;
        if( *Q == 0 ) break;
        Q++; T++;
    }
}

void norm_filename(char *filename) {
    char *C = filename;
    while( (*C != 0) && (*C == ' ' || *C < 14)) removecharat(C);
    if(*C == 0) return;
    while(*C != 0) {
        if(*C == '/') *C = '\\';
        if(*C < 14) { *C = '\0'; break; }
        C++;
    }
    while( *(--C) == ' ' ) *C = '\0';
}

void removelastslash(char *path) {
    char *slash = strrchr(path, '\\');
    if(slash != NULL) {
        if( *(++slash) == 0 ) *(--slash) = '\0';
    }
}

void removeslashes(char *path) {
    char *C = path;
    while(*C != 0) {
        if(*C == '/' || *C == '\\' || *C == ':') removecharat(C);
        else C++;
    }
}

void removestartdots(char *path) {
    char *C = path;
    while(*C == '.' || *C == ' ') removecharat(C);
}

void removesemicolons(char *path) {
    char *C = path;
    while(*C != 0) {
        if(*C == ':') removecharat(C);
        else C++;
    }
}

void toremotepath(char *path) {
    char *C = path;
    int L = strlen(working_directory);
    if( strncmp( C, working_directory, L )==0 ) {
        while( L-- ) removecharat(C);
    }
    if( *C == 0 ) {
        *(C++) = '/';
        *C = '\0';
        return;
    }    
    while( *C != 0 ) {
        if(*C == '\\') *C = '/';
        C++;
    }
}

void safefilename(char *path) {
    norm_filename(path);
    removeslashes(path);
    removestartdots(path);
    GetCurrentDirectory(MAX_PATH, pathbuf);
    removelastslash(pathbuf);    
    strcat(pathbuf,"/");
    strcat(pathbuf,path);    
    norm_filename(pathbuf);    
    strcpy(path, pathbuf);
    printf("file:%s\n",path);
}

void remove_till_parent_folder(char *path) {
    size_t len = strlen(path);
    if (len == 0) return;
    while (len > 0 && path[len-1] == '\\') path[--len] = '\0';
    while (len > 0 && path[len-1] != '\\') path[--len] = '\0';
}

void cmdtoupper(char *buf) {
    char *C = buf;
    int U;
    while(*C != 0 && *C == ' ') removecharat(C);
    while(*C != 0 && *C != ' ')
    {
        if(*C < 14) { *C = '\0'; break; }
        if(*C >= 'a') {
            U = *C;
            *C = (char)(U - 32);
            }
        C++;
    }
}

void get_file_attributes(char *output, WIN32_FIND_DATA *find_data) {
    char file_permissions[11] = "-rwxrwxrwx";
    if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        file_permissions[0] = 'd';
    }
    static char formatted_time[30];
    SYSTEMTIME st;
    FileTimeToSystemTime(&find_data->ftLastWriteTime, &st); // Now using FileTimeToSystemTime
    char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    sprintf(formatted_time, "%s %02d %04d %02d:%02d", months[st.wMonth - 1], st.wDay, st.wYear, st.wHour, st.wMinute);
    //unsigned long long size = ((unsigned long long)find_data->nFileSizeHigh << 32) | find_data->nFileSizeLow;
    DWORD size = find_data->nFileSizeLow;    // 4GB
    sprintf(output, "%s    1 user     group %12lu %s %s\r\n", file_permissions, size, formatted_time, find_data->cFileName);
}

void list_directory() {
    strcpy(list_buffer, "");
    hFind = FindFirstFile("*", &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        send_response("550 Directory listing failed\r\n");
        return;
    }
    do {
        if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
            get_file_attributes(entry, &find_data);
            strcat(list_buffer, entry);
        }
    } while (FindNextFile(hFind, &find_data));
    FindClose(hFind);
    strcat(list_buffer, "\r\n"); // Ensure proper termination
    if(utf8) {
        convert_to_utf8(list_buffer);
        strcpy(list_buffer, buffer);
    }
    SOCKET transfer_sock = get_data_socket();
    if (transfer_sock == INVALID_SOCKET) {
        send_response("425 Can't open data connection\r\n");
        return;
    }
    send_response("150 Here comes the directory listing\r\n");
    send(transfer_sock, list_buffer, strlen(list_buffer), 0);
    closesocket(transfer_sock);
    send_response("226 Directory send OK.\r\n");
}

void alna() {
    int id = 0;
    strcpy(list_buffer, "220-Alternative names of files and folders\r\n");
    hFind = FindFirstFile("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                sprintf(entry, "{%d}=%s\r\n", (++id), find_data.cFileName);            
                strcat(list_buffer, entry);
            }
        } while (FindNextFile(hFind, &find_data));
        FindClose(hFind);
    }
    strcat(list_buffer, "220 Ok\r\n");    
    if(utf8) {
        convert_to_utf8(list_buffer);
        strcpy(list_buffer, buffer);
    }    
    send_response(list_buffer);
}

void alnw() {
    int id = 0;
    strcpy(list_buffer, "220-Alternative 2-byte char names of files and folders\r\n");
    hFindW = FindFirstFileW(L"*", &find_dataW);
    if (hFindW != INVALID_HANDLE_VALUE) {
        do {
            wcscpy( utf16buffer, find_dataW.cFileName );
            if( w16utf8() == 0 ) strcpy(pathbuf, buffer);
            else strcpy(pathbuf, "");
            if (strcmp(pathbuf, ".") != 0 && strcmp(pathbuf, "..") != 0) {
                sprintf(entry, "{w%d}=%s\r\n", (++id), pathbuf);            
                strcat(list_buffer, entry);
            }
        } while (FindNextFileW(hFindW, &find_dataW));
        FindClose(hFindW);
    }
    strcat(list_buffer, "\r\n Can do sequence:\r\n   RNFR {wNr}\r\n   RNTO newname\r\n");
    strcat(list_buffer, " to rename the encoded files and folders.\r\n");
    strcat(list_buffer, "220 Ok\r\n");
    send_response(list_buffer);
}

int ck_alna( char *filename ) {
    int id = 0, n = 0, fA, fW;
    char *A = filename;
    norm_filename(A);
    use_w16 = (( *(A++)=='{' && *(A--)=='w' ) ? 1 : 0);
    hFindW = FindFirstFileW(L"*", &find_dataW);
    hFind = FindFirstFile("*", &find_data);
    if (hFindW != INVALID_HANDLE_VALUE && hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                id++;
                if( use_w16 ) {
                    sprintf(entry, "{w%d}", id);
                } else {
                    sprintf(entry, "{%d}", id);
                }
                if( strcmp( filename, entry ) == 0 ) {
                    strcpy( filename, find_data.cFileName );
                    wcscpy( filenameW, find_dataW.cFileName );
                    printf("{%d} is \"%s\"\n", id, find_data.cFileName);
                    n++;
                    break;
                }
            }
            fW = (FindNextFileW(hFindW, &find_dataW) ? 1 : 0 );
            fA = (FindNextFile(hFind, &find_data) ? 1 : 0 );
            if( fW != fA ) return 0;
        } while (fW);
        FindClose(hFindW);
        FindClose(hFind);
    }
    return n;
}

int is100ascii( char c ) {
    return ((( c=='.' ) || (c >='0' && c <= '9') ||  (c >='A' && c <= 'Z') || (c >='a' && c <= 'z')) ? 1 : 0);
}

int canexactthisname( char *actual, char *mask ) {
    char *C = actual, *B = mask;
    return ( ( strncmp( C, B, strlen(B) ) == 0 ) ? 1 : 0 );
}

int canbethisname( char *actual, char *mask ) {
    char *C = actual, *B = mask;
    // mask is like 8 chars.
    while( *B != 0 ) {
        if( *B != *C ) {
            if( is100ascii(*C) || is100ascii(*B) ) return 0;
        }
        // still may be, so so
        C++; B++;
    }
    return 1;
}

int filename_err_correction( char *filename ) {
    int cnt = 0;
    hFind = FindFirstFile("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char *f = find_data.cFileName;
            if( canexactthisname( f, filename ) ) {
                strcpy( buff1, f );
                cnt++;
            }
        } while (FindNextFile(hFind, &find_data));
        FindClose(hFind);
    }
    if( cnt == 1 ) return 1;
    cnt = 0;
    hFind = FindFirstFile("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char *f = find_data.cFileName;
            if( canbethisname( f, filename ) ) {
                strcpy( buff1, f );
                cnt++;
            }
        } while (FindNextFile(hFind, &find_data));
        FindClose(hFind);
    }
    return cnt;
}

void nlst() {
    list_directory();
}

void pwd() {
    GetCurrentDirectory(MAX_PATH, pathbuf);
    printf("get current: %s\n",pathbuf);
    toremotepath(pathbuf);
    sprintf(buffer, "257 \"%s\" is the current directory\r\n", pathbuf);
    send_response(buffer);
}

void retr(char *filename) {
    strcpy( filenm, filename );
    norm_filename(filenm);
    removeslashes(filenm);
    SOCKET transfer_sock = get_data_socket();
    if (transfer_sock == INVALID_SOCKET) {
        send_response("425 Can't open data connection\r\n");
        return;
    }
    if( ck_alna(filename) ) {
        strcpy( filenm, filename );
    }
    file = fopen(filenm, transfer_mode ? "rb" : "rt");
    if(!file) {
        if( filename_err_correction(filenm) == 1 ) {
            strcpy( filenm, buff1 );
            file = fopen(filenm, transfer_mode ? "rb" : "rt");
        }
    }
    if (!file) {
        send_response("550 File not found\r\n");
    } else {
        send_response("150 Opening data connection\r\n");
        int bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            if(transfer_mode==0 && utf8) {
                memcpy( buff2, buffer, bytes_read );
                convert_to_utf8( buff2 );
            }
            send(transfer_sock, buffer, bytes_read, 0);
        }
        fclose(file);
        printf("[INFO] Sent file: %s\n", (char *)filenm);
        closesocket(transfer_sock);
        send_response("226 Transfer complete\r\n");
    }
}

void size(char *filename) {
    int a = ck_alna(filename);
    strcpy( pathbuf, filename );
    if(!a) {
        norm_filename(pathbuf);
        removeslashes(pathbuf);
    }
    file = fopen(pathbuf, "rb");
    if(!file) {
        if( filename_err_correction(pathbuf) == 1 ) {
            strcpy( pathbuf, buff1 );
            file = fopen(pathbuf, "rb");
        }
    }
    if (file) {
        fseek(file, 0L, SEEK_END);
        sprintf(buffer, "213 %ld\r\n", ftell(file));
        fclose(file);
        send_response(buffer);
        return;
    }    
    send_response("550 File not found\r\n");
}

void stor(char *filename) {
    strcpy( filenm, filename );
    norm_filename(filenm);
    removeslashes(filenm);
    SOCKET transfer_sock = get_data_socket();
    if (transfer_sock == INVALID_SOCKET) {
        send_response("425 Can't open data connection\r\n");
        return;
    }
    file = fopen(filenm, transfer_mode ? "wb" : "wt");
    if (!file) {
        send_response("550 Failed to open file for writing\r\n");
    } else {
        send_response("150 Ready to receive file\r\n");
        int bytes_received;
        while ((bytes_received = recv(transfer_sock, buffer, BUFFER_SIZE, 0)) > 0) {
            fwrite(buffer, 1, bytes_received, file);
            // ignores UTF8 to Windows ASCII...
            // client can send in binary mode
        }
        fclose(file);
        printf("[INFO] Received file: %s\n", (char *)filenm);
        closesocket(transfer_sock);
        send_response("226 Transfer complete\r\n");
    }
}
            
void pasv() {
    int h1, h2, h3, h4;
    sscanf(client_ip, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);
    int port = 50000 + (rand() % 1000);
    pasv_sock = socket(AF_INET, SOCK_STREAM, 0);
    pasv_addr.sin_family = AF_INET;
    pasv_addr.sin_addr.s_addr = INADDR_ANY;
    pasv_addr.sin_port = htons(port);
    bind(pasv_sock, (struct sockaddr*)&pasv_addr, sizeof(pasv_addr));
    listen(pasv_sock, 1);
    sprintf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", h1, h2, h3, h4, port / 256, port % 256);
    printf("port:%ld\n",port);
    send_response(buffer);
}

void port(char *params) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(params, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    active_addr.sin_family = AF_INET;
    active_addr.sin_addr = client_addr.sin_addr;
    active_addr.sin_port = (ULONG)htons(p1 * 256 + p2);
    printf("port:%ld\n",active_addr.sin_port);
    connect(data_sock, (struct sockaddr*)&active_addr, sizeof(active_addr));
    send_response("200 PORT command successful\r\n");
}

void goroot() {
    SetCurrentDirectory(working_directory);
}

int setpathinternal( char *folder ) {
    char *a = folder, *b = working_directory, *c = buff2;
    norm_filename(a);
    int good = 1;
    // touppercase
    int a1 = *a; if(a1 >= 97) a1 -= 32; *a = (char)a1;
    int b1 = *b; if(b1 >= 97) b1 -= 32; *b = (char)b1;
    if( (*a != *b) &&  (perm & p_fCHANGEROOT) ) {
        good = 0;
        for( int drive = 1; drive < 26; drive++ ) {
            if( _chdrive( drive ) == 0 ) {
                GetCurrentDirectory(MAX_PATH,buff2);
                int c1 = *c; if(c1 >= 97) c1 -= 32; *c = (char)c1;
                if( *a == *c ) {
                    good = 1;
                    break;
                }
            }
        }
    }
    if( good ) {
        good = ( SetCurrentDirectory(folder) ? 1 : 0 );
        }
    GetCurrentDirectory(MAX_PATH, working_directory);
    printf("currently: %s\n", working_directory);
    return good;
}

void setfolder( char *folder ) {
    int tryresult = ( SetCurrentDirectory(folder) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(folder) == 1 ) {
            strcpy( folder, buff1 );
            tryresult = ( SetCurrentDirectory(folder) ? 1 : 0 );
        }
    }
    if( tryresult ) {
        send_response("250 Directory successfully changed\r\n");
    } else {
        send_response("550 Failed to change directory\r\n");
    }
    // verify to be sure
    GetCurrentDirectory(MAX_PATH, pathbuf);
    if( strncmp( pathbuf, working_directory, strlen(working_directory) ) != 0 ) {
        goroot();
    }
    GetCurrentDirectory(MAX_PATH, pathbuf);
    printf("set current: %s\n",pathbuf);
}

void cdup() {
    GetCurrentDirectory(MAX_PATH, pathbuf);
    norm_filename(pathbuf);
    if (strcmp(pathbuf, working_directory) == 0) {
        send_response("250 Already at root directory\r\n");
        return;
    }
    remove_till_parent_folder(pathbuf);
    setfolder(pathbuf);
}

void cwd(char *path) {
    if( !ck_alna(path) ) {
        strcpy(pathbuf,path);
        norm_filename(pathbuf);
        toremotepath(pathbuf);
        if( strcmp( pathbuf, ".." )==0 ) {
            cdup();
            return;
        }
        if( strcmp( pathbuf, "." )==0 ) {
            send_response("250 NOOP\r\n");
            return;
        }
        if( strcmp( pathbuf, "/" )==0 ) {
            GetCurrentDirectory(MAX_PATH, pathbuf);
            toremotepath(pathbuf);
            if ( strcmp( pathbuf, "/" )==0 ) {
                send_response("250 Already at root directory\r\n");
                return;
            }
            setfolder(working_directory);
            return;
        }
        norm_filename(path);
        removelastslash(path);
        removesemicolons(path);
        if( path[0]=='\\' ) {
            strcpy(pathbuf, working_directory);
            strcat(pathbuf, path);
            setfolder(pathbuf);
            return;
        }
    }
    setfolder(path);
}

void mkd(char *path) {
    safefilename(path);
    if (CreateDirectory(path, NULL)) {
        send_response("257 Directory created\r\n");
    } else {
        send_response("550 Failed to create directory\r\n");
    }
}

void rmd(char *path) {
    if(!ck_alna(path)) safefilename(path);
    int tryresult = ( RemoveDirectory(path) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(path) == 1 ) {
            strcpy( path, buff1 );
            tryresult = ( RemoveDirectory(path) ? 1 : 0 );
        }
    }
    if (tryresult) {
        send_response("250 Directory removed\r\n");
    } else {
        send_response("550 Failed to remove directory\r\n");
    }
}

void dele(char *path) {
    if(!ck_alna(path)) safefilename(path);
    int tryresult = ( DeleteFile(path) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(path) == 1 ) {
            strcpy( path, buff1 );
            tryresult = ( DeleteFile(path) ? 1 : 0 );
        }
    }
    if (tryresult) {
        send_response("250 File deleted\r\n");
    } else {
        send_response("550 Failed to delete file\r\n");
    }
}

void rnfr(char *old_name) {
    strcpy( filenm, old_name );
    safefilename( filenm );
    if(ck_alna(old_name) && (!use_w16)) strcpy( filenm, old_name );
    send_response("350 Ready for RNTO\r\n");
}

void rnto(char *old_name, char *new_name) {
    int attributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL;
    int tryresult = 0;
    if( use_w16 ) {
        norm_filename(new_name);
        if(!utf8_w16(new_name)) {
            SetFileAttributesW(filenameW, attributes);
            wprintf(L"renaming to: %s\n", utf16buffer);
            tryresult = ( MoveFileW(filenameW, utf16buffer) ? 1 : 0 );
            printf("RESULT=%d\n",tryresult);
        }
    }
    safefilename(new_name);
    if( !tryresult ) {
        SetFileAttributes(old_name, attributes);
        tryresult = ( MoveFile(old_name, new_name) ? 1 : 0 );
    }
    if( !tryresult ) {
        if( filename_err_correction(old_name) == 1 ) {
            strcpy( old_name, buff1 );
            SetFileAttributes(old_name, attributes);
            tryresult = ( MoveFile(old_name, new_name) ? 1 : 0 );
        }
    }
    if (tryresult) {
        send_response("250 File or folder renamed successfully\r\n");
    } else {
        send_response("550 Failed to rename file or folder\r\n");
    }
}

void site(char *cmd) {
    char mode[4];
    cmdtoupper(cmd);
    if (sscanf(cmd, "CHMOD %3s %s", mode, filenm) != 2) {
        send_response("500 Invalid STAT command format\r\n");
        return;
    }
    int attributes = FILE_ATTRIBUTE_ARCHIVE;
    if (strcmp(mode, "777") == 0) {
        attributes |= FILE_ATTRIBUTE_NORMAL;
    } else if (strcmp(mode, "444") == 0) {
        attributes |= FILE_ATTRIBUTE_READONLY;
    } else if (strcmp(mode, "000") == 0) {
        attributes |= FILE_ATTRIBUTE_HIDDEN;
    } else {
        send_response("500 Unsupported chmod mode\r\n");
        return;
    }
    if(!ck_alna(filenm) && (!use_w16)) safefilename(filenm);
    int tryresult = 0;
    if( use_w16 ) {
        tryresult = ( SetFileAttributesW(filenameW, attributes) ? 1 : 0 );
    }
    if( !tryresult ) {
        tryresult = ( SetFileAttributes(filenm, attributes) ? 1 : 0 );
    }
    if( !tryresult ) {
        if( filename_err_correction(filenm) == 1 ) {
            strcpy( filenm, buff1 );
            tryresult = ( SetFileAttributes(filenm, attributes) ? 1 : 0 );
        }
    }
    if (tryresult) {
        send_response("200 File attributes changed\r\n");
    } else {
        send_response("550 Failed to change file attributes\r\n");
    }
}

void feat() {
    strcpy( buffer,"211-Features:\r\n" );
    strcat( buffer,"- USER PASS LIST NLST PORT PASV CWD CDUP XCUP \r\n");
    strcat( buffer,"- SYST FEAT PWD XPWD STAT QUIT BYE RETR STOR SIZE \r\n");
    strcat( buffer,"- TYPE MKD XMKD RMD XRMD DELE RNFR RNTO HELP NOOP \r\n");
    strcat( buffer,"- OPTS UTF8 ON/OFF\r\n");
    strcat( buffer,"- SITE CHMOD 777/444/000 repeatedly\r\n");
    strcat( buffer,"- ALNA - alternative names\r\n");
    strcat( buffer,"- ALNW - altern.names with 2-byte char support\r\n");
    strcat( buffer,"211 End\r\n" );
    send_response( buffer);
}

void help() {
    send_response("220 Help is FEAT and readme\r\n");
}

void stat() {
    sprintf(buffer, "211-FTP server status:\r\n Connected to %s\r\n TYPE: ", server_ip);
    if(transfer_mode == 0) {
        strcat(buffer, "ASCII");
    } else {
        strcat(buffer, "BINARY");
    }
    strcat(buffer, "\r\n File transfer MODE: Stream\r\n211 End of status\r\n");
    send_response(buffer);
}

int optsonoff( char *set ) {
    cmdtoupper(set);
    if( strncmp( set, "ON", 2 ) == 0 ) {
        utf8 = 1;
        send_response("200 UTF8 encoding enabled\r\n");
        return 1;
    }
    if( strncmp( set, "OFF", 3 ) == 0 ) {
        utf8 = 0;
        send_response("200 UTF8 encoding disabled\r\n");
        return 1;
    }
    return 0;
}
    
void opts( char *opt ) {
    cmdtoupper(opt);
    if( strncmp( opt, "UTF8", 4 ) == 0 ) {
        if( optsonoff( opt + 5 ) ) return;
    }
    send_response("500 Command not recognized\r\n");
}

void ifloggedin( char *s ) {
    strcpy( buffer, s );
    if( (perm & (p_fUSER|p_fPSW)) == 0 ) {
        strcat( buffer, ", logged in" );
        perm |= p_fLOGGEDIN;
        }
    strcat( buffer, "\r\n" );
    send_response( buffer );
}

void user( char *us ) {
    norm_filename(us);
    if(us[0]=='\0') {
        strcpy(us,"Anonymous");
        printf("%s\n",us);
    }
    if( ( perm & p_fUSER ) && ( strcmp( us, usr ) != 0 ) ) {
        send_response("530 Incorrect username\r\n");
        }
    else {
        if( perm & p_fUSER ) perm ^= p_fUSER;
        ifloggedin( "331 Username ok" );
        }
}

void pass( char *pw ) {
    norm_filename(pw);
    if(pw[0]=='\0') {
        strcpy(pw,"guest");
        printf("%s\n",pw);
    }
    if( ( perm & p_fPSW ) && ( strcmp( pw, psw ) != 0 ) ) {
        send_response("530 Incorrect password\r\n");
        }
    else {
        if( perm & p_fPSW ) perm ^= p_fPSW;
        ifloggedin( "230 Password ok" );
        }
}

void root( char *drive ) {
    if( setpathinternal( drive ) ) {
        send_response("250 Root changed\r\n");
    } else {
        send_response("550 Failed to change root\r\n");
    }
}

void client() {
    control_port = 21;
    data_port = 20;
    passive_mode = 1;
    transfer_mode = 0;
    utf8 = 0;
    strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
    printf("[INFO] Client connected: %s\n", client_ip);
    send_response("220 Simple FTP Server\r\n");
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;
        
        cmdtoupper(buffer);
        if( strncmp(buffer, "PASS", 4) == 0 ) {
            printf("[CMD] PASS\n");
            }
        else {
            printf("[CMD] %s\n", buffer);
            }
        if (strncmp(buffer, "OPTS", 4) == 0) {
            opts( buffer + 5 );
        } else if (strncmp(buffer, "USER", 4) == 0) {
            user( buffer + 5 );
        } else if (strncmp(buffer, "PASS", 4) == 0) {
            pass( buffer + 5 );
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "LIST", 4) == 0) {
            list_directory();
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "NLST", 4) == 0) {
            nlst();
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "ALNA", 4) == 0) {
            alna();    // not standard
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "ALNW", 4) == 0) {
            alnw();    // not standard
        } else if (strncmp(buffer, "PORT", 4) == 0) {
            passive_mode = 0;
            port(buffer + 5);
        } else if (strncmp(buffer, "PASV", 4) == 0) {
            passive_mode = 1;
            pasv();
        } else if ( (pm(p_fLOGGEDIN)) && strncmp(buffer, "CWD", 3) == 0) {
            cwd(buffer + 4);
        } else if ( (pm(p_fLOGGEDIN)) && strncmp(buffer, "CDUP", 4) == 0) {
            cdup();            
        } else if ( (pm(p_fLOGGEDIN)) && strncmp(buffer, "XCUP", 4) == 0) {
            cdup();
        } else if ( (pm(p_fLOGGEDIN|p_fCHANGEROOT)) && strncmp(buffer, "ROOT", 4) == 0) {
            root(buffer + 5);    // not standard, hacking tool
        } else if (strncmp(buffer, "SYST", 4) == 0) {
            send_response("215 Windows TinyC made ftp\r\n");
        } else if (strncmp(buffer, "FEAT", 4) == 0) {
            feat();
        } else if ( (pm(p_fLOGGEDIN)) && strncmp(buffer, "PWD", 3) == 0) {
            pwd();
        } else if ( (pm(p_fLOGGEDIN)) && strncmp(buffer, "XPWD", 4) == 0) {
            pwd();
        } else if (strncmp(buffer, "STAT", 4) == 0) {
            stat();
        } else if (strncmp(buffer, "QUIT", 4) == 0 || strncmp(buffer, "BYE", 3) == 0) {
            send_response("221 Bye.\r\n");
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "RETR ", 5) == 0) {
            retr(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "STOR ", 5) == 0) {
            stor(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fREAD)) && strncmp(buffer, "SIZE", 4) == 0) {
            size(buffer + 5);
        } else if (strncmp(buffer, "TYPE I", 6) == 0) {
            transfer_mode = 1;
            send_response("200 Binary mode set\r\n");
        } else if (strncmp(buffer, "TYPE A", 6) == 0) {
            transfer_mode = 0;
            send_response("200 ASCII mode set\r\n");
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "MKD ", 4) == 0) {
            mkd(buffer + 4);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "XMKD ", 5) == 0) {
            mkd(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "RMD ", 4) == 0) {
               rmd(buffer + 4);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "XRMD ", 5) == 0) {
            rmd(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "DELE ", 5) == 0) {
            dele(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "RNFR ", 5) == 0) {
            rnfr(buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "RNTO ", 5) == 0) {
            rnto(filenm, buffer + 5);
        } else if ( (pm(p_fLOGGEDIN|p_fWRITE)) && strncmp(buffer, "SITE ", 5) == 0) {
            site(buffer + 5);
        } else if (strncmp(buffer, "HELP", 4) == 0) {
            help();
        } else if (strncmp(buffer, "NOOP", 4) == 0) {
            send_response("250 NOOP\r\n");
        } else {
            send_response("500 Unknown command\r\n");
            }
    }
    printf("[INFO] Client disconnected\n");
    closesocket(client_sock);
    goroot();
}

void hello() {
 static char *hello_txt[] = {
 (char *)" --------------------------------\n",
 (char *)" This is an ftp server as a tool\n",
 (char *)"   for very old windows PCs.\n",
 (char *)" Can connect and get or put a file.\n",
 (char *)" Inbuilt cmd.exe>ftp.exe works ok.\n",
 (char *)"\n",
 (char *)" ChatGPT, 2025\n",
 (char *)" --------------------------------\n",
 (char *)"Start with /? or /h for help\n",
 (char *)"\0",
 };
 for( int i = 0; hello_txt[i][0] != '\0' ; i++ ) {
    printf( hello_txt[i] );
    }
 }

void options() {
 static char *opts_txt[] = {
 (char *)"Security options:\n",
 (char *)"Syntax: ftp_xp [/R][/W][/D][/A][/U=username][/P]\n",
 (char *)"  R - Read permission, list and download\n",
 (char *)"  W - Write permission, can upload, rename, delete\n",
 (char *)"  D - can go command ROOT C:\\ or D:\\ or any\n",
 (char *)"  A - total admin permission, can do all\n", 
 (char *)"  U - Can set one required user name, avoids guests\n",
 (char *)"  P - Requires to enter password, console input on start\n", 
 (char *)"\n",
 (char *)"Al least something, otherwise the disclaimer\n",
 (char *)"\0",
 };
 for( int i = 0; opts_txt[i][0] != '\0' ; i++ ) {
    printf( opts_txt[i] );
    }
 }

int main(int argc, char **argv) {
    
    //options from commandline
    if (argc > 1) {
        for( int a = 1; a < argc;  a++ ) {
            char* o = argv[a];
            if( *(o++) == '/' ) {
                if( *o == '?' || *o == 'h' ) {
                    options();
                    return 0;
                }
                if( *o == 'A' ) perm |= (p_fREAD|p_fWRITE|p_fCHANGEROOT);                
                if( *o == 'R' ) perm |= p_fREAD;
                if( *o == 'W' ) perm |= p_fWRITE;
                if( *o == 'D' ) perm |= p_fCHANGEROOT;
                if( *o == 'P' ) perm |= p_fPSW;
                if( *(o++) == 'U' && *(o++) == '='  ) {
                    sscanf( o, "%s",  usr );
                    perm |= p_fUSER;
                    }
                }
            }
        }
    perm_saved = perm;
    hello();
    if(perm & p_fPSW) {
        printf("Enter password 6 chars\n:");
        for( int j=0; j<6; j++ ) psw[j] = _getch();
        psw[6] = '\0';
    }
    
    GetModuleFileNameA(NULL, app_directory, MAX_PATH);
    *(strrchr(app_directory, '\\')) = '\0';
    strcpy( working_directory, app_directory ); 
    
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    client_len = sizeof(client_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(control_port);
    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    get_server_ip(server_ip, sizeof(server_ip));
    printf("[INFO] FTP Server started at ftp://%s:%d\n", server_ip, control_port);
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock != INVALID_SOCKET) {
            client();
        }
        perm = perm_saved;
        setpathinternal(app_directory);
    }
    closesocket(server_sock);
    WSACleanup();
    return 0;
}
