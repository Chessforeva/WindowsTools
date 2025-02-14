/*

 FTP server for Windows XP
 
   in simplest old C style
   8-bit chars only in filenames
 
 MS Visual C++ 2010 version, it is Microsoft
 
 Chessforeva, 2025
 
*/

#include "stdafx.h"

#define _WIN32_WINNT 0x0501 // Ensure compatibility with Windows XP
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "wsock32.lib")

#define Kb100 (100*1024)
#define BUFFER_SIZE Kb100

char buffer[Kb100];
char buff1[Kb100];
char pathbuf[Kb100];
char list_buffer[Kb100*6];    // 600Kbytes
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
WIN32_FIND_DATAA find_data;
HANDLE hFind;
int control_port = 21;
int data_port = 20;
int passive_mode = 1;
int transfer_mode = 0; // 0 = ASCII, 1 = Binary
WSADATA wsa_data;
SOCKET server_sock = INVALID_SOCKET, client_sock = INVALID_SOCKET;
SOCKET data_sock = INVALID_SOCKET, pasv_sock = INVALID_SOCKET;
char app_directory[3000];
char working_directory[3000] = "C:\\";

#define p_fLOGGEDIN 1
#define p_fREAD 2
#define p_fWRITE 4
#define p_fCHANGEROOT 8
#define p_fUSER 16
#define p_fPSW 32

int perm = 0;            // permission
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

void send_response(char *message) {
    send(client_sock, message, strlen(message), 0);
    printf("%s\n",message);
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
    GetCurrentDirectoryA(3000, pathbuf);
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

void get_file_attributes(char *output, WIN32_FIND_DATAA *find_data) {
    char file_permissions[11] = "-rwxrwxrwx";
    if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        file_permissions[0] = 'd';
    }
    static char formatted_time[30];
    SYSTEMTIME st;
    FileTimeToSystemTime(&find_data->ftLastWriteTime, &st); // Now using FileTimeToSystemTime
    char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    sprintf(formatted_time, "%s %02d %04d %02d:%02d", months[st.wMonth - 1], st.wDay, st.wYear, st.wHour, st.wMinute);
    DWORD size = find_data->nFileSizeLow;    // 4GB
    sprintf(output, "%s    1 user     group %12lu %s %s\r\n", file_permissions, size, formatted_time, find_data->cFileName);
}

void list_directory() {
    strcpy(list_buffer, "");
    hFind = FindFirstFileA("*", &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        send_response("550 Directory listing failed\r\n");
        return;
    }
    do {
        if (strcmp((char *)find_data.cFileName, ".") != 0 && strcmp((char *)find_data.cFileName, "..") != 0) {
            get_file_attributes(entry, &find_data);
            strcat(list_buffer, entry);
        }
    } while (FindNextFileA(hFind, &find_data));
    FindClose(hFind);
    strcat(list_buffer, "\r\n"); // Ensure proper termination
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
    hFind = FindFirstFileA("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                sprintf(entry, "{%d}=%s\r\n", (++id), find_data.cFileName);            
                strcat(list_buffer, entry);
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
    strcat(list_buffer, "220 Ok\r\n");       
    send_response(list_buffer);
}

int ck_alna( char *filename ) {
    int id = 0, n = 0, fA, fW;
    char *A = filename;
    norm_filename(A);
    hFind = FindFirstFileA("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                id++;
                sprintf(entry, "{%d}", id);
                if( strcmp( filename, entry ) == 0 ) {
                    strcpy( filename, find_data.cFileName );
                    printf("{%d} is \"%s\"\n", id, find_data.cFileName);
                    n++;
                    break;
                }
            }
            fA = (FindNextFileA(hFind, &find_data) ? 1 : 0 );
        } while (fA);
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
    hFind = FindFirstFileA("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char *f = find_data.cFileName;
            if( canexactthisname( f, filename ) ) {
                strcpy( buff1, f );
                cnt++;
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
    if( cnt == 1 ) return 1;
    cnt = 0;
    hFind = FindFirstFileA("*", &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char *f = find_data.cFileName;
            if( canbethisname( f, filename ) ) {
                strcpy( buff1, f );
                cnt++;
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
    return cnt;
}
void nlst() {
    list_directory();
}

void pwd() {
    GetCurrentDirectoryA(3000, pathbuf);
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
    active_addr.sin_port = (u_long)htons(p1 * 256 + p2);
    printf("port:%ld\n",active_addr.sin_port);
    connect(data_sock, (struct sockaddr*)&active_addr, sizeof(active_addr));
    send_response("200 PORT command successful\r\n");
}

void goroot() {
    SetCurrentDirectoryA(working_directory);
}

int setpathinternal( char *folder ) {
    char *a = folder, *b = working_directory, *c = buff2;
    int c1;
    norm_filename(a);
    int good = 1;
    // touppercase
    int a1 = *a; if(a1 >= 97) a1 -= 32; *a = (char)a1;
    int b1 = *b; if(b1 >= 97) b1 -= 32; *b = (char)b1;
    if( (*a != *b) &&  (perm & p_fCHANGEROOT) ) {
        good = 0;
        int drive = 1 + (a1-65);
        if( drive > 0 && drive < 26 ) {
            if( _chdrive( drive ) == 0 ) {
                GetCurrentDirectoryA(3000,buff2);
                c1 = *c; if(c1 >= 97) c1 -= 32; *c = (char)c1;
                if( *a == *c ) {
                    good = 1;
                }
            }
        }
        if(!good) {
            // can throw an error on a device
            for( drive = 1; drive < 26; drive++ ) {
                if( _chdrive( drive ) == 0 ) {
                    GetCurrentDirectoryA(3000,buff2);
                    c1 = *c; if(c1 >= 97) c1 -= 32; *c = (char)c1;
                    if( *a == *c ) {
                         good = 1;
                         break;
                    }
                }
            }
        }
    }
    if( good ) {
        good = ( SetCurrentDirectoryA(folder) ? 1 : 0 );
        }
    if( good ) {
        GetCurrentDirectoryA(3000, working_directory);      
    }
    else {
        SetCurrentDirectoryA(working_directory);
    }
    printf("currently: %s\n", working_directory);
    return good;
}

void setfolder( char *folder ) {
    int tryresult = ( SetCurrentDirectoryA(folder) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(folder) == 1 ) {
            strcpy( folder, buff1 );
            tryresult = ( SetCurrentDirectoryA(folder) ? 1 : 0 );
        }
    }
    if( tryresult ) {
        send_response("250 Directory successfully changed\r\n");
    } else {
        send_response("550 Failed to change directory\r\n");
    }
    // verify to be sure
    GetCurrentDirectoryA(3000, pathbuf);
    if( strncmp( pathbuf, working_directory, strlen(working_directory) ) != 0 ) {
        goroot();
    }
    GetCurrentDirectoryA(3000, pathbuf);
    printf("set current: %s\n",pathbuf);
}

void cdup() {
    GetCurrentDirectoryA(3000, pathbuf);
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
            GetCurrentDirectoryA(3000, pathbuf);
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
    if (CreateDirectoryA(path, NULL)) {
        send_response("257 Directory created\r\n");
    } else {
        send_response("550 Failed to create directory\r\n");
    }
}

void rmd(char *path) {
    if(!ck_alna(path)) safefilename(path);
    int tryresult = ( RemoveDirectoryA(path) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(path) == 1 ) {
            strcpy( path, buff1 );
            tryresult = ( RemoveDirectoryA(path) ? 1 : 0 );
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
    int tryresult = ( DeleteFileA(path) ? 1 : 0 );
    if( !tryresult ) {
        if( filename_err_correction(path) == 1 ) {
            strcpy( path, buff1 );
            tryresult = ( DeleteFileA(path) ? 1 : 0 );
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
    if(ck_alna(old_name)) strcpy( filenm, old_name );
    send_response("350 Ready for RNTO\r\n");
}

void rnto(char *old_name, char *new_name) {
    int attributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL;
    int tryresult = 0;
    safefilename(new_name);
    if( !tryresult ) {
        SetFileAttributesA(old_name, (DWORD)attributes);
        tryresult = ( MoveFileA(old_name, new_name) ? 1 : 0 );
    }
    if( !tryresult ) {
        if( filename_err_correction(old_name) == 1 ) {
            strcpy( old_name, buff1 );
            SetFileAttributesA(old_name, (DWORD)attributes);
            tryresult = ( MoveFileA(old_name, new_name) ? 1 : 0 );
        }
    }
    if (tryresult) {
        send_response("250 File renamed successfully\r\n");
    } else {
        send_response("550 Failed to rename file\r\n");
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
    if(!ck_alna(filenm)) safefilename(filenm);
    int tryresult = 0;
    if( !tryresult ) {
        tryresult = ( SetFileAttributesA(filenm, (DWORD)attributes) ? 1 : 0 );
    }
    if( !tryresult ) {
        if( filename_err_correction(filenm) == 1 ) {
            strcpy( filenm, buff1 );
            tryresult = ( SetFileAttributesA(filenm, (DWORD)attributes) ? 1 : 0 );
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

    
void opts( char *opt ) {
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
    strcpy(client_ip, inet_ntoa(client_addr.sin_addr));
    printf("[INFO] Client connected: %s\n", client_ip);
    send_response("220 Simple FTP Server\r\n");
    perm = p_fLOGGEDIN | p_fREAD |  p_fWRITE | p_fCHANGEROOT;
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
 (char *)" 8bit chars in filenames, Windows XP\n",
 (char *)"\n",
 (char *)" ChatGPT, 2025\n",
 (char *)" --------------------------------\n",
 (char *)"\0",
 };
 for( int i = 0; hello_txt[i][0] != '\0' ; i++ ) {
    printf( hello_txt[i] );
    }
 }

int main(int argc, char **argv) {
    hello();
    GetModuleFileNameA(NULL, app_directory, 3000);
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
        setpathinternal(app_directory);
    }
    closesocket(server_sock);
    WSACleanup();
    return 0;
}

