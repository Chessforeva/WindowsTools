/*

  A tool to use after
    DIR *.* /s /a > o.txt

 Syntax:
    dir2htm o.txt o.htm

 After can browse folders and
  see consumed space.
 Files and folders above 20Mb only.
 Not a fastest one, but ok.

 Compiled on MS Visual Studio 2022 C++,
  for Windows.

 Chessforeva, feb.2025

*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#pragma warning(disable: 4996)

#define SIZE_ABOVE_IN_LIST (20*1024*1024)

#define MALLOC_ENTITIES_FOLDERS (8*1000*1000)
#define MALLOC_ENTITIES_FILES (80*1000*1000)

// 800Mb for strings
#define MALLOC_STRINGS (800*1000*1000)

#define SMALL_BUFFER_5MB (5*1024*1024)

#define Kb100 (100*1024)

typedef struct Entity {
    unsigned int Parent_I;
    char* Name;
    unsigned long long Size;
    char Lastmod_date[20];
    int Folder;
    int Junction;
    int nolist;
    unsigned int nr;
    unsigned int up_nr;
    unsigned int nextFolder_I;
} Entity;

typedef struct FolderEntity {
    unsigned int nr_I;
    char* Path;
} FolderEntity;

int stage = 0, cur_len = 0;
int dpos = 0, tpos = 0, szpos = 0, nmpos = 0, sz_l = 0, dt_l = 0;

char line[Kb100];

wchar_t utf16buffer[Kb100];   // 200Kbytes
char buffer[Kb100];
char buffr2[Kb100];
char buffr3[Kb100];
char buf[Kb100];
char bu2[Kb100];

char filenameTXT[3000];
char filenameHTM[3000];

char cur_folder[3000];

int err_flag = 0;

Entity* E0, * E;
FolderEntity* F0, * F, * f_last;

char* P = cur_folder;
char* S0, * S;
char* L0, * L;
unsigned int Slen;

unsigned int Ecnt;
unsigned int Fcnt;
unsigned int Parent_I;
FILE* file;

const char* colHex[5] = { "#c3b4a1", "#c3b4a1", "#ff683e", "#0aee01", "#8a2be2" };
const char* colCSS[5] = { "v", "v", "b", "u", "w" };
int colpal;

char cperc[12] = "00000000000";
char dperc[12] = "00000000000";

const char* digs = "0123456789";

unsigned long long rdlines = 0;

void add_last_slash(char* path) {
    char* C = path;
    if (*C == '\0') return;
    while (*C != '\0') C++;
    if (*(--C) != '\\') {
        *(++C) = '\\';
        *(++C) = '\0';
    }
}

void removecharat(char* s) {
    char* T = s, * Q = s;
    T++;
    while (*Q != 0) {
        *Q = *T;
        if (*Q == 0) break;
        Q++; T++;
    }
}

void str_trunc(char* path) {
    char* C = path;
    if (*C == '\0') return;
    while (*C != '\0') {
        if (*C < 14) *C = '\0';
        else C++;
    }
    while (*(--C) == ' ') *C = '\0';
}

void trunc_all(char* path) {
    char* C = path;
    while ((*C != 0) && (*C == ' ' || *C < 14)) removecharat(C);
    str_trunc(C);
}

void addString(char* s) {
    while (1) {
        *S = *s;
        if (*S == '\0') break;
        S++; s++;
        Slen++;
    }
    *(++S) = '\0';
}

void analyze_dir(char* line) {

    char* C, * Q, * X, * P = buffr3;
    int d, l;
    char c, g;
    FolderEntity* f;

    C = strstr(line, "Directory of ");
    if (C != NULL) {
        C = &C[13];
        strcpy(P, C);
        trunc_all(P);
        add_last_slash(P);
        if (!cur_len) {
            strcpy(cur_folder, P);
            cur_len = strlen(P);
            F->nr_I = Fcnt;
            F->Path = S;
            addString(P);
            f_last = F++; Fcnt++;
            Parent_I++;
        }
        else {

            l = strlen(P);
            X = &P[l - 1];
            g = *X; *X = '\0';
            C = strrchr(P, '\\');
            if (C != NULL) {
                c = *(++C); *C = '\0';
                f = F;
                d = 0;
                while (1) {
                    f--;
                    if (strcmp(f->Path, P) == 0) {
                        d = 1;
                        break;
                    }
                    if (f->nr_I == 0) break;
                }
                *C = c;
                if (!d) err_flag |= 32;
            }
            *X = g;
            F->nr_I = Fcnt;
            F->Path = S;
            addString(P);
            f_last = F++; Fcnt++;
            Parent_I++;
            if (err_flag & 32) {
                printf("\nMissing subfolder entity, try DIR *.* /s /a\n");
                printf("%s\n", f_last->Path);
                return;
            }
        }
    }
    else {
        C = &line[tpos + 2];
        if ((*C == ':') && (strchr(digs, *(--C)) != NULL)) {
            C = &line[nmpos];
            X = strchr(C, '[');
            if (X != NULL) {
                *X = '\0';      // JUNCTION or SYMLINKD
                E->Junction = 1;
            }
            E->Parent_I = (Parent_I - 1);
            E->nolist = 0;
            trunc_all(C);
            E->Name = S;
            addString(C);
            C = &line[dpos];
            strncpy(E->Lastmod_date, C, dt_l);
            C = &line[nmpos]; *C = '\0';
            C = &line[szpos];
            E->Folder = ((*C == '<') ? 1 : 0);
            Q = C; d = sz_l;
            while (d--) {
                if (strchr(digs, *Q) == NULL) removecharat(Q);
                else Q++;
            }
            if (*C == '\0') E->Size = 0;
            else {
                sscanf_s(C, "%llu", &E->Size);
            }
            E->nr = Ecnt++;
            E++;
        }
    }

}

void read_dir_txt() {

    char* C, * Q, * W;
    int i;
    char* ln = L0;
    int accum = 0;

    printf("Reading file...\n");

    Parent_I = 0;
    Ecnt = 0; Fcnt = 0;
    E = E0; F = F0;

    while (fgets(line, sizeof(line), file)) {        // Read line by line
        if (!((rdlines++) % 100000)) printf(".");
        if (rdlines)
            trunc_all(line);
        if (stage == 0 && strchr(line, ':') != NULL) {
            i = 0;
            C = line;
            while (1) {
                if (*C == '\0') break;
                if (*C == ':') {
                    Q = C; W = C;
                    if ((strchr(digs, *(--Q)) != NULL) && (strchr(digs, *(++W)) != NULL)) {
                        tpos = i - 2;          // time place
                    }
                }
                if (*C == '<') {
                    Q = C; W = ++Q;
                    if (*Q == 'D' && *(++W) == 'I') {
                        szpos = i;          // size place
                        while (*C != ' ') { i++; C++; }
                        while (*C == ' ') { i++; C++; }
                        nmpos = i;           // names place
                    }
                }
                i++; C++;
            }

            if (dpos < tpos < szpos < nmpos) {
                sz_l = nmpos - szpos;
                dt_l = tpos - dpos + 5;
                stage = 1;                // ready to process files list 

                // process lines accumuled
                ln = L0;
                while (accum--) {
                    C = buffr2;
                    while (1) {
                        *C = *(ln++);
                        if (*C == '\0') break;
                        C++;
                    }
                    analyze_dir(buffr2);
                }
            }
        }

        // save line if no complete information
        if (stage == 0) {
            C = line;            // save line in buffer
            i = 0;
            while (1) {
                *ln = *C;
                if (*C < 14) break;
                C++; ln++; i++;
                if (i > 2990) {
                    err_flag |= 64;
                    printf("\nWrong length of lines.\n");
                    return;
                }
            }
            *(ln++) = '\0';
            accum++;
        }
        else {
            analyze_dir(line);   // we can simply analyze
        }
        if (err_flag & 32) return;
    }

    printf("\nStrLen:%lu, FoldCnt:%lu, FileCnt:%u\n", Slen, Fcnt, Ecnt);
    E--; F--;

}

unsigned long long consumedTotal() {
    unsigned long long TotalSize = 0;
    Entity* e = E0;
    while (e->Parent_I == 0) {
        TotalSize += e->Size;
        e++;
    }
    return TotalSize;
}

void calc_sizes() {

    unsigned int i = Ecnt;
    unsigned int par_I, par_nr;
    Entity* e, * e1;
    FolderEntity* f, * f1;
    char* H, * h;
    unsigned int k = 0, perc = 0, pi = 0;

    e = E; f = F;
    while ((i--) > 0) {
        k++;
        perc = (100 * k) / Ecnt;
        pi = perc / 10;
        if (cperc[pi] == '0') {
            cperc[pi] = '1';
            printf("%d%c ", perc, '%');
        }
        if (strcmp(e->Name, ".") != 0 && strcmp(e->Name, "..") != 0) {
            while (f->nr_I != e->Parent_I) f--;
            strcpy(buf, f->Path);
            H = strrchr(buf, '\\');
            h = strchr(buf, '\\');
            if ((H == h) || (f->nr_I == 0)) {
                buf[0] = '\0';    // nothing above
            }
            else {
                // perform path split into folder and subfolder
                *(H--) = '\0';
                while (1) {
                    if (*H == '\\') break;
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
                while (strcmp(f->Path, buf) != 0) {
                    if (f->nr_I == 0) {
                        printf("\nerror finding folder [%s]\n", buf);
                        err_flag |= 1;
                        return;
                    }
                    f--;
                }

                // now find folder and the right entity
                while (!(e->Folder) || (e->Parent_I != f->nr_I) || (strcmp(e->Name, bu2) != 0)) {
                    if ((e->Parent_I < f->nr_I) || (e->nr == 0)) {
                        printf("\nerror finding entity [%s]\n", bu2);
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
                if (!e1->Junction) {
                    e->Size += e1->Size;
                }
                par_nr = e->nr;

                e = e1; f = f1;

                // save back .. folder, saves pointer information
                while ((e->Parent_I != 0) && (e->Parent_I == e1->Parent_I)) {
                    if (strcmp(e->Name, "..") == 0) {
                        e->nextFolder_I = par_I;
                        e->up_nr = par_nr;
                    }
                    if (strcmp(e->Name, ".") == 0) e->nextFolder_I = 0;
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
    Entity* e, * e1;
    int rmv = 0;
    unsigned int k = 0, perc = 0, pi = 0;

    e = E;
    while ((i--) > 0) {
        k++;
        perc = (100 * k) / Ecnt;
        pi = perc / 10;
        if (dperc[pi] == '0') {
            dperc[pi] = '1';
            printf("%d%c ", perc, '%');
        }
        if (e->Parent_I != l_Parent) {
            rmv = 1;
            l_Parent = e->Parent_I;
        }
        if (strcmp(e->Name, ".") == 0) {
            if (rmv) e->nolist = 1;
        }
        else if (strcmp(e->Name, "..") == 0) {
            if (rmv) {
                e->nolist = 1;
                e1 = e;
                // save back .. folder, saves pointer information
                while (e->nr != e1->up_nr) e--;
                e->nolist = 1;
                e = e1;
            }
        }
        else {
            if (e->Size < SIZE_ABOVE_IN_LIST) e->nolist = 1;
            else if (!e->nolist) rmv = 0;
        }
        e--;
    }
}

void make_consumed_str(unsigned long long value, char* retval) {
    unsigned long long KB = 1024, MB = 1024 * KB, GB = 1024 * MB, TB = 1024 * GB;

    colpal = 0;
    if (value >= TB) {
        sprintf(retval, "%llu TB", (value / TB));
        colpal = 4;
    }
    else if (value >= GB) {
        sprintf(retval, "%llu GB", (value / GB));
        colpal = 3;
    }
    else if (value >= MB) {
        sprintf(retval, "%llu Mb", (value / MB));
        colpal = 2;
    }
    else if (value >= KB) sprintf(retval, "%llu Kb", (value / KB));
    else sprintf(retval, "%llu", value);
}

void to_safe_html(char* s, int oneC) {
    char* C = buf;
    while (1) {
        *C = *s;
        char Ch = *C;
        int c = (int)Ch;
        if (c == 0) break;
        if (c < 32 || (c > 32 && c < 43) || c == 47 || c == 60 || c == 62 || (c > 90 && c < 97) || c>122) {
            if (oneC) *C = '?';
            else {
                sprintf(C, "{%02x}", c);
                while (*C != 0) C++;
                C--;
            }
        }
        C++; s++;
    }
}

void make_html() {

    Entity* e = E0;
    FolderEntity* f = F0, * f2;
    unsigned int i = Ecnt;
    unsigned int ni;
    char atrb[10];

    file = fopen(filenameHTM, "w");
    if (!file) {
        printf("Error creating file: %s\n", filenameHTM);
        return;
    }

    fprintf(file, "<HTML><HEAD><STYLE>\n");
    fprintf(file, ".a{background-color:aqua;cursor:pointer}\n.c{font-family:courier}\n");
    fprintf(file, ".y{background-color:yellow}\n.d{visibility:hidden;height:0px}\n");
    fprintf(file, ".v{background-color:#c3b4a1}\n.b{background-color:#ff683e}\n");
    fprintf(file, ".u{background-color:#0aee01}\n.w{background-color:#8a2be2}\n");
    to_safe_html(F0->Path, 0);
    fprintf(file, "</STYLE></HEAD>\n<BODY ONLOAD='T(%lu);S(0,\"%s\")'>\n", Ecnt + 99, buf);
    make_consumed_str(consumedTotal(), buf);
    fprintf(file, "<DIV id=\"DT\" class=\"%s\" style=\"width:100px\">%s</DIV>\n", colCSS[colpal], buf);
    fprintf(file, "<DIV id=\"DP\" class=\"c\"></DIV>\n");
    fprintf(file, "<DIV id=\"DB\" class=\"c\">\n");
    while (i > 0) {
        while (e->nolist && i > 0) { e++; i--; }
        if (i == 0) break;
        fprintf(file, "<DIV id=\"D%lu\" class=\"d\">\n", e->Parent_I);
        fprintf(file, "<TABLE>");
        while (f->nr_I != e->Parent_I) f++;
        while (e->Parent_I == f->nr_I) {
            if (!e->nolist) {
                atrb[0] = '\0';
                if (e->Folder) {
                    // find path of next folder
                    ni = e->nextFolder_I;
                    f2 = f;
                    if (ni == 0) f2 = F0;
                    else
                    {
                        while (f2->nr_I != ni) {
                            if (ni < f2->nr_I) f2--;
                            else f2++;
                        }
                    }
                    to_safe_html(f2->Path, 0);
                    fprintf(file, "<TR onclick='S(%lu,\"%s\")' class=\"a\">", e->nextFolder_I, buf);
                    strcat(atrb, "D");
                    if (e->Junction) {
                        strcat(atrb, "J");
                    }
                }
                else {
                    fprintf(file, "<TR class=\"y\">");
                }
                fprintf(file, "<TD>%s</TD>", atrb);
                to_safe_html(e->Name, 0);
                fprintf(file, "<TD><DIV id=\"V%lu\">%s</DIV></TD>", (Ecnt - i), buf);
                make_consumed_str(e->Size, buf);
                if (colpal < 2) {
                    fprintf(file, "<TD>%s</TD>", buf);
                }
                else {
                    fprintf(file, "<TD class=\"%s\">%s</TD>", colCSS[colpal], buf);
                }
                fprintf(file, "<TD>%s</TD>\n", E->Lastmod_date);
                fprintf(file, "</TR>");
            }
            e++;
            i--;
        }
        fprintf(file, "</TABLE></DIV>\n");
    }
    fprintf(file, "</DIV><BR><BR>\n");

    // add a javascript mouse click and decoding of text on load
    fprintf(file, "<SCRIPT>\nfunction G(J){return document.getElementById(J)}\n");
    fprintf(file, "function D(p){for(u=\"\",j=0;j<p.length;j++){x=p[j];u+=(x=='{'?");
    fprintf(file, "String.fromCharCode(parseInt(\"0x\"+p.substr((++j),2),16))+((j+=2)?'':''):x)}}\n");
    fprintf(file, "function T(j){while(j>=0){s=G(\"V\"+j);if(s!=null){u=s.innerHTML;D(u);s.innerHTML=u};j--}}");
    fprintf(file, "function H(){s=G(\"DE\").style;s.visibility=\"hidden\";s.height=\"0px\"}");
    fprintf(file, "function Z(K,L){s=G(\"D\"+K).style;s.visibility=(L?\"visible\":\"hidden\");s.height=(L?\"400px\":\"0px\")}\n");
    fprintf(file, "s={};A=[];I=0;j=0;u=\"\";x=0;\nfunction S(n,p){Z(I,0);I=n;Z(I,1);D(p);G(\"DP\").innerHTML=u}</SCRIPT>\n");

    fprintf(file, "</BODY></HTML>\n");
    fclose(file);
}

void display() {
    make_consumed_str(consumedTotal(), buf);
    printf("Consumed: %s\n", buf);
}

int main(int argc, char** argv) {

    if (argc < 3) {
        printf("This tool takes an output file o.txt after Windows command\n");
        printf(" DIR *.* /s /a > o.txt\n");
        printf("and creates a browsable o.htm file of consumed space.\n");
        printf("Syntax:\n dir2htm o.txt o.htm\n\n");
        return 0;
    }
    else {
        strcpy(filenameTXT, argv[1]);
        strcpy(filenameHTM, argv[2]);
    }

    S0 = (char*)malloc(MALLOC_STRINGS);
    E0 = (Entity*)malloc(sizeof(Entity) * MALLOC_ENTITIES_FILES);
    F0 = (FolderEntity*)malloc(sizeof(FolderEntity) * MALLOC_ENTITIES_FOLDERS);
    L0 = (char*)malloc(SMALL_BUFFER_5MB);

    S = S0;

    printf("Parsing file %s for consumed space above 20Mb.\n", filenameTXT);

    errno_t err = fopen_s(&file, filenameTXT, "rt");     //qdir.txt
    if (err) {
        printf("Could not open file. Exiting.\n");
        return 1;
    }

    read_dir_txt();
    fclose(file);

    if (err_flag & 4) printf("Out of memory, too large paths and filenames.\n");
    else {
        if (!(err_flag & (32 + 64))) {
            printf("Calculating sizes of folders...\n");
            calc_sizes();
            printf("\nDetailed...\n");
            remove_small_sizes();
            printf("\nWriting %s...\n", filenameHTM);
            make_html();
            display();
        }
    }
    if (err_flag & 1) printf("There were errors while processing.\n");
    printf("Ok\n");
    free(L0);
    free(F0);
    free(E0);
    free(S0);
    return 0;
}
