Intended to observe the content of an old PC. Or a new one.
Unfortunately it does not find hidden system folders.
Then use DIR command.

Modern windows can list all files and folder by using command.
 DIR *.* /s /a:h > DLIST.TXT

Older windows could not do it properly.
The "dir" command at the beginning was a DOS command for 8 chars max.filenames.
This produced lists of files (on FAT disks) named as C:\Progra~1\... .
UTF8 wasn't the main standard till XP times, as I remember.
NTFS introduced 2-byte character filenames in various encodings for different languages.
This had "splitted" programming C functions into two cases as
 CreateDirectoryA (8-bit), CreateDirectoryW (16-bit), and lots of others, of course.
Encodings became a mess of tables - ascii, ansi, latin, windows, koi, iso, etc.

The selection and opening of a folder or a file became tricky when using an old 8-bit
function for these 2-byte character names, the system could open a file or
simply return an error "File or folder not found".

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

The output file is easy to read sequentially. Such a small bonus.

After can open "view" folder and run list2htm.exe tool.
This creates a browsable dlist.htm from the DLIST.TXT file (place in current folder)
to spot easily most space consuming folders and find large caches and tmp-files on disks.
But not the hidden Windows system files.

Advice:
If You wonder what takes so much space on Your harddisk where Windows is, then
open Properties of the Disk, then "Disk Cleanup" and press button "Clean up system files".
You will discover Gb of updates accumulated during years. Clear it.
The other thing is Google Chrome can accumulate Gb in cache.
Also various development platforms as Visual Studio simply collect files in folders.
Anyway, there is a .py script for spotting:
 https://github.com/Chessforeva/various / dir_size
This does it well.

Maybe will make something else someday.
