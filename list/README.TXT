Intended to observe the content of an old PC.

Modern windows can list all files and folder by using command

 DIR *.* /s /a:h > DLIST.TXT

Older windows could not do it properly.
The dir command was DOS based 8 chars max. and produced filenames as C:\Progra~1\...
Also, when encoded filenames appeared in XP, there was not UTF8 standard and
NTFS 2-byte character filenames in various encodings for different languages
 have "splitted" programming C functions as
 CreateDirectoryA (8-bit), CreateDirectoryW (16-bit), and lots of others, of course.
Encodings became a mess of tables - ascii, ansi, latin, windows, koi, iso, etc.

The selection and opening of a folder or a file became risky using an old 8-bit
function for these 2-byte character names, the system could open or
simply return a error "File or folder not found".

So, introducing tools:

+ list95.exe    for Win95, compiled with TinyC compiler (FindFileFirstA function)
                (remember that modern windows deletes it)
                Of course, Win98,XP. But 8-bit only.

+ list8bit.exe  for WinXP, MS Visual C++ 2010 compiled.  (FindFileFirstA function)
                It is 8-bit characters only filenames, does not use newer functions.
                May be useful for Win98 too, do not know.

+ list_w16.exe  for WinXP, MS Visual C++ 2010 compiled.  (FindFileFirstW function)
                Everything is 2-byte characters in filenames.
                Does not produce errors on folder switching.
                But only if NTFS supports these functions. Maybe PC has FAT disks.
                This can be used for modern PCs too while Microsoft Win32 applications are valid.
                But can use DIR command instead.
