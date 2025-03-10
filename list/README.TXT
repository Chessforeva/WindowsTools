Intended to observe the content of an old PC. Or a new one too.

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

+ list_x64.exe  for Windows 10, 11. MSStudio 2022 C++ compiled. 16bit wide filenames
                (new, made it too, because it works)

+ list_hs.exe [<folder>]     for Windows 10, 11. MSStudio 2022 C++ compiled. 16bit wide filenames
                             This searches all folders possible including hidden system.
                             Good to see all consumed space, but maybe bad to spot
                             inaccessible folders. It does not try set as current,
                             only reads folder information. Anyway, other approach may give
                             different results. This is the most recent build.
                             So, use list_hs C:\  and list2htm  after to brow results.

--- older win-------------
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

Also there is a tool dir2htm.exe that creates a browsable o.htm file after DIR 
  DIR *.* /s /a > o.txt
  dir2htm.exe o.txt o.htm

This is for consumed diskspace observation, not a detailed finding of hidden small files.
Sometimes I just need to check my disks. By the way, DIR command does not find all files
in one listing, should find by parts with additional options for hidden: DIR *.* /s /a:h
But these cases are not developed in dir2htm currently. Only DIR *.* /s /a listing of the most.

Advice:
If You wonder what takes so much space on Your harddisk where Windows is, then
open Properties of the Disk, then "Disk Cleanup" and press button "Clean up system files".
You will discover Gb of updates accumulated during years. Clear it.
The other thing is Google Chrome can accumulate Gb in cache.
Also various development platforms as Visual Studio simply collect files in folders.
Anyway, there is a .py script for spotting:
 https://github.com/Chessforeva/various / dir_size
This does it well.

The current version tries to set the folder and then read files in it. This does not work on
hidden system folders. It would be right to search files without changing current folder. Sure.
But, this way dlist.htm easily spots viruses, which mark their folders as hidden system parts.
Take a look under the list for unaccessed folders for names known as trojans or malwares.
Also imprtant is the last modification time of a folder. If it is modified by a virus, then all
folders with the same modification time should be deleted, or renamed, or manually reviewed.
Slow and painful work guaranteed with powershell, regedit and windows permissions.
Anyway, keep PC clean. Windows Defender is good, it really fights.
