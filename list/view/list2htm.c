/*

 Addition to the "list" project.
 https://github.com/Chessforeva/WindowsTools/ list
 
 As a second phase, makes a nice dlist.htm
  from DLIST.TXT

 After can browse folders and
  see consumed space.
 Files and folders above 20Mb only.
 
 Compiled on gcc for Windows.
 
 Chessforeva, feb.2025
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#define SIZE_ABOVE_IN_LIST (20*1024*1024)

#define MALLOC_ENTITIES_FOLDERS (2*1000*1000)
#define MALLOC_ENTITIES_FILES (20*1000*1000)

// 800Mb for strings
#define MALLOC_STRINGS (800*1000*1000)

typedef struct Entity {
    unsigned int Parent_I;
    char *Name;
    unsigned long long Size;
    char Lastmod_date[20];
    int Folder;
    int Hidden;
    int nolist;
    unsigned int nr;
    unsigned int up_nr;
    unsigned int nextFolder_I;
} Entity;

typedef struct FolderEntity {
    unsigned int nr_I;
    char *Path;
} FolderEntity;

char line[3000];
char cur_folder[3000];
char buf[3000];
char bu2[3000];
int err_flag = 0;

Entity *E0, *E;
FolderEntity *F0, *F;
char *P = cur_folder;
char *S0, *S;
unsigned int Slen;

unsigned int Ecnt;
unsigned int Fcnt;
unsigned int Parent_I;
FILE *file;

const char *filenameTxt = "DLIST.TXT";
const char *filenameHtm = "dlist.htm";

const char* colHex[5] = { "#c3b4a1", "#c3b4a1", "#ff683e", "#0aee01", "#8a2be2" };
const char* colCSS[5] = { "v", "v", "b", "u", "w" };
int colpal;

char cperc[12] = "00000000000";
char dperc[12] = "00000000000";

void add_last_slash( char *path ) {
    char *C = path;
    if( *C == '\0' ) return;
    while( *C != '\0' ) C++;
    if(*(--C) != '\\' ) {
        *(++C) = '\\';
        *(++C) = '\0';
    }
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

void str_trunc( char *path ) {
    char *C = path;
    if( *C == '\0' ) return;
    while( *C != '\0' ) {
        if( *C<14 ) *C = '\0';
        else C++;
    }
    while( *(--C) == ' ' ) *C = '\0';
}

void trunc_all( char *path ) {
    char *C = path;
    while( (*C != 0) && (*C == ' ' || *C < 14)) removecharat(C);
    str_trunc(C);
}

void addString( char *s ) {
    while(1) {
        *S = *s;
        if(*S == '\0') break;
        S++; s++;
        Slen++;
    }
    *(++S) = '\0';
}

void parse_dlist() {
    
    file = fopen(filenameTxt, "r");
    if (!file) {
        printf("Error opening file: %s\n", filenameTxt);
        return;
    }

    S = S0; Slen = 0;
    
    Parent_I = 0;
    Ecnt = 0; Fcnt = 0;
    cur_folder[0] = '\0';
    E = E0; F = F0;    
    
    while (fgets(line, sizeof(line), file)) {
        str_trunc(line);
        if (line[0] == '\n' || line[0] == '\0') continue;
        strcpy( buf, line );
        buf[5] = '\0';
        if( strcmp( buf, "Error" ) == 0 ) {
            err_flag |= 2;
            continue;
        }

        if ( strchr(line,':') != NULL && strchr(line,'\\') != NULL ) {
            strcpy(P, line);        // cur_folder
            trunc_all(P);
            add_last_slash(P);
            F->nr_I = Fcnt;
            F->Path = S;
            addString(P);
            F++; Fcnt++;
            Parent_I++;
        } else {
            char mmm[4], dd[3], yyyy[5];
            char *L = line;
            E->Parent_I = (Parent_I-1);
            E->Folder = ((*(L++) == 'D') ? 1 : 0);
            E->Hidden = ((*(L++) == 'H') ? 1 : 0);
            E->nolist = 0;
            sscanf(L, "%llu %s %s %s", &E->Size, &mmm, &dd, &yyyy);
            
            // primitive location of filename
            sscanf(L, "%llu", &E->Size);
            while(*L == ' ') L++;
            while(*L != ' ') L++;    // skip size
            while(*L == ' ') L++;
            while(*L != ' ') L++;    // skip mmm
            while(*L == ' ') L++;
            while(*L != ' ') L++;    // skip dd
            while(*L == ' ') L++;
            while(*L != ' ') L++;    // skip yyyy
            while(*L == ' ') L++;        // locate name of the file
            trunc_all(L);
            E->Name = S;
            addString(L);
            sprintf( E->Lastmod_date, "%s %s %s", mmm, dd, yyyy ); 
            E->nr = Ecnt++;
            E++;
        }
    
        if(Slen >= (MALLOC_STRINGS - 2000)) {
            err_flag |= 4;
            return;
            }
    }
    fclose(file);
    printf("%d entities\n",Ecnt);
    printf("StrLen:%lu, FoldCnt:%lu, FileCnt:%u\n", Slen, Fcnt, Ecnt);
    E--; F--;
}

unsigned long long consumedTotal() {
    unsigned long long TotalSize = 0;
    Entity *e = E0;
    while( e->Parent_I == 0 ) {
        TotalSize += e->Size;
        e++;
    }
    return TotalSize;
}

void calc_sizes() {
    
    unsigned int i = Ecnt;
    unsigned int par_I, par_nr;
    Entity *e, *e1;
    FolderEntity *f, *f1;
    char *H, *h;
    unsigned int k=0,perc=0,pi=0;

    e = E; f = F;
    while( (i--) > 0 ) {
        k++;
        perc=(100*k)/Ecnt;
        pi=perc/10;
        if(cperc[pi]=='0') {
            cperc[pi]='1';
            printf("%d%c ",perc,'%');
        }
        if( strcmp(e->Name,".") != 0 && strcmp(e->Name,"..") != 0 ) {
            while( f->nr_I != e->Parent_I ) f--;
            strcpy( buf, f->Path );
            H = strrchr( buf, '\\' );
            h = strchr( buf, '\\' );
            if( (H == h) || (f->nr_I == 0) ) {
                buf[0] = '\0';    // nothing above
                }
            else {
                // perform path split into folder and subfolder
                *(H--) = '\0';
                while(1) {
                    if( *H == '\\' ) break;
                    H--;
                }
                H++;
                strcpy(bu2, H);        // subfolder
                *H = '\0';            // buf contains folder
                e1 = e; f1 = f;
                trunc_all(bu2);
                trunc_all(buf);
                
                //debug case
                //printf("{%s}{%s}[%s][%s]\n",e->Name,f->Path,buf, bu2);
                
                // now find folder and the right entity
                while( strcmp( f->Path, buf ) != 0 ) {
                    if( f->nr_I == 0 ) {
                        printf( "\nerror finding folder [%s]\n", buf);
                        err_flag |= 1;
                        return;
                        }
                    f--;
                }
                
                // now find folder and the right entity
                while( !(e->Folder) || (e->Parent_I != f->nr_I) || (strcmp( e->Name, bu2 ) != 0) ) {
                    if( (e->Parent_I < f->nr_I) || (e->nr == 0) ) {
                        printf( "\nerror finding entity [%s]\n", bu2);
                        err_flag |= 1;
                        return;
                        }
                    e--;
                }
                //debug case
                //printf("|%s|\n",e->Name);
                
                // Found the entity of folder to the next folder, cd e->Name
                // also saves pointer information
                par_I = e->Parent_I;
                e->nextFolder_I = e1->Parent_I;
                e->Size += e1->Size;
                par_nr = e->nr;
                
                e = e1; f = f1;
                
                // save back .. folder, saves pointer information
                while( (e->Parent_I != 0 ) && (e-> Parent_I == e1-> Parent_I ) ) {
                    if( strcmp( e->Name, ".." )==0 ) {
                        e->nextFolder_I = par_I;
                        e->up_nr = par_nr;
                        }
                    if( strcmp( e->Name, "." )==0 ) e->nextFolder_I = 0;
                    e--;
                }
                e = e1;
                
            }
        }
        e--;
    }
}


void remove_small_sizes() {
    
    unsigned int i = Ecnt;
    unsigned int l_Parent = 0;
    Entity *e, *e1;
    int rmv = 0;
    unsigned int k=0,perc=0,pi=0;
    
    e = E;
    while( (i--) > 0 ) {
        k++;
        perc=(100*k)/Ecnt;
        pi=perc/10;
        if(dperc[pi]=='0') {
            dperc[pi]='1';
            printf("%d%c ",perc,'%');
        }
        if( e->Parent_I != l_Parent ) {
            rmv = 1;
            l_Parent = e->Parent_I;
        }
        if( strcmp(e->Name,".") == 0) {
            if(rmv) e->nolist = 1;
            }
        else if( strcmp(e->Name,"..") == 0) {
            if(rmv) {
                e->nolist = 1;
                e1 = e;
                // save back .. folder, saves pointer information
                while( e->nr != e1->up_nr ) e--;
                e->nolist = 1;
                e = e1;
                }
            }
        else {
            if(e->Size< SIZE_ABOVE_IN_LIST) e->nolist = 1;
            else if(!e->nolist) rmv = 0;
            }
        e--;
    }
}

void make_consumed_str( unsigned long long value, char *retval ) {
    unsigned long long KB = 1024, MB = 1024*KB, GB = 1024*MB, TB = 1024*GB;
    
    colpal = 0;
    if( value >= TB ) {
        sprintf( retval, "%llu TB", (value / TB) );
        colpal = 4;
    }
    else if( value >= GB ) {
        sprintf( retval, "%llu GB", (value / GB) );
        colpal = 3;
    }
    else if( value >= MB ) {
        sprintf( retval, "%llu Mb", (value / MB) );
        colpal = 2;
    }    
    else if( value >= KB ) sprintf( retval, "%llu Kb", (value / KB) );
    else sprintf( retval, "%llu", value );    
}

void to_safe_html( char *s, int oneC ) {
    char *C = buf;
    while(1) {
        *C = *s;
        char Ch = *C;
        int c = (int)Ch;
        if(c==0) break;
        if( c<32 || (c>32 && c<43) || c==47 || c==60 || c==62 || (c>90 && c<97) || c>122 ) {
            if(oneC) *C = '?';
            else {
                sprintf( C, "{%02x}", c );
                while(*C != 0) C++;
                C--;
                }
            }
        C++; s++; 
    }
}


void make_html() {
    
    Entity *e = E0;
    FolderEntity *f = F0, *f2;
    unsigned int i = Ecnt;
    unsigned int ni;
    char atrb[10];
            
    file = fopen(filenameHtm, "w");
    if (!file) {
        printf("Error creating file: %s\n", filenameHtm);
        return;
    }

    fprintf( file, "<HTML><HEAD><STYLE>\n");
    fprintf( file, ".a{background-color:aqua;cursor:pointer}\n.c{font-family:courier}\n" );
    fprintf( file, ".y{background-color:yellow}\n.d{visibility:hidden;height:0px}\n" );
    fprintf( file, ".v{background-color:#c3b4a1}\n.b{background-color:#ff683e}\n" );
    fprintf( file, ".u{background-color:#0aee01}\n.w{background-color:#8a2be2}\n" );
    to_safe_html( F0->Path, 0 );
    fprintf( file, "</STYLE></HEAD>\n<BODY ONLOAD='T(%lu);S(0,\"%s\")'>\n", Ecnt, buf );
    make_consumed_str( consumedTotal(), buf );
    fprintf( file, "<DIV id=\"DT\" class=\"%s\" style=\"width:100px\">%s</DIV>\n", colCSS[colpal], buf );
    fprintf( file, "<DIV id=\"DP\" class=\"c\"></DIV>\n" );
    fprintf( file, "<DIV id=\"DB\" class=\"c\">\n" );
    while( i > 0 ) {
        while( e->nolist && i>0 ) { e++; i--; }
        if(i==0) break;
        fprintf( file, "<DIV id=\"D%lu\" class=\"d\">\n", e->Parent_I );
        fprintf( file, "<TABLE>" );
        while( f->nr_I != e->Parent_I ) f++;
        while( e->Parent_I  == f->nr_I ) {
            if(!e->nolist) {
                atrb[0] = '\0';
                if(e->Folder) {
                    // find path of next folder
                    ni = e->nextFolder_I;
                    f2 = f;
                    if( ni == 0 ) f2 = F0;
                    else 
                        {
                        while( f2->nr_I != ni ) {
                            if( ni < f2->nr_I ) f2--;
                            else f2++;
                            }
                        }
                    to_safe_html( f2->Path, 0 );
                    fprintf( file, "<TR onclick='S(%lu,\"%s\")' class=\"a\">", e->nextFolder_I, buf );
                    strcat( atrb, "D" );
                }
                else {
                    fprintf( file, "<TR class=\"y\">");    
                }
                if(e->Hidden) strcat( atrb, "H" );
                fprintf( file, "<TD>%s</TD>", atrb );
                to_safe_html( e->Name, 0 );
                fprintf( file, "<TD><DIV id=\"V%lu\">%s</DIV></TD>", (Ecnt-i), buf );
                make_consumed_str( e->Size, buf );
                if(colpal<2) {
                    fprintf( file, "<TD>%s</TD>", buf );
                    }
                else {
                    fprintf( file, "<TD class=\"%s\">%s</TD>", colCSS[colpal], buf );
                    }
                fprintf( file, "<TD>%s</TD>\n", E->Lastmod_date );
                fprintf( file, "</TR>" );
            }
            e++;
            i--;
        }
        fprintf( file, "</TABLE></DIV>\n" );
    }
    fprintf( file, "</DIV>\n" );

    // add a javascript mouse click and decoding of text on load
    fprintf( file, "<SCRIPT>\nfunction G(J){return document.getElementById(J)}\n");
    fprintf( file, "function D(p){for(u=\"\",j=0;j<p.length;j++){x=p[j];u+=(x=='{'?");
    fprintf( file, "String.fromCharCode(parseInt(\"0x\"+p.substr((++j),2),16))+((j+=2)?'':''):x)}}\n");
    fprintf( file, "function T(j){while(j>=0){s=G(\"V\"+j);if(s!=null){u=s.innerHTML;D(u);s.innerHTML=u};j--}}");
    fprintf( file, "function Z(K,L){s=G(\"D\"+K).style;s.visibility=(L?\"visible\":\"hidden\");s.height=(L?\"400px\":\"0px\")}\n");
    fprintf( file, "s={};A=[];I=0;j=0;u=\"\";x=0;\nfunction S(n,p){Z(I,0);I=n;Z(I,1);D(p);G(\"DP\").innerHTML=u}</SCRIPT>\n");    
    
    fprintf( file, "</BODY></HTML>\n" );
    fclose(file);
}

void display() {
    make_consumed_str( consumedTotal(), buf );
    printf( "Consumed: %s\n", buf );
}

int main() {
    S0 = malloc( MALLOC_STRINGS );
    E0 = malloc( sizeof(Entity) * MALLOC_ENTITIES_FILES );
    F0 = malloc( sizeof(FolderEntity) * MALLOC_ENTITIES_FOLDERS );
    printf("Parsing file %s for consumed space above 20Mb.\n", filenameTxt);
    parse_dlist();
    if(err_flag&4) printf("Out of memory, too large paths and filenames.\n");
    else {
        printf("Calculating sizes of folders...\n");
        calc_sizes();
        printf("\nDetailed...\n");
        remove_small_sizes();
        printf("\nWriting %s...\n", filenameHtm);
        make_html();
        display();
        }
    if(err_flag&1) printf("There were errors while processing.\n");
    if(err_flag&2) printf("Hidden system folders not accessible,\nor filename decoding errors in list.\n");
    printf("Ok\n");
    free(F0);
    free(E0);
    free(S0);
    return 0;
}
