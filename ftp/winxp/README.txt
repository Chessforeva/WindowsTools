About:

This is an FTP tool to access old Windows PCs.
Not a server for online users as we know it.
To get out photos and content from once abandoned stuff.

+  ftp_xp - TinyC, uses ws2_32.lib, also 2-byte filenames (Some modern Windows deletes the executable without a notice)
+  ftp_vc10 - MS Visual C++ 2010, uses ws2_32.lib, also 2-byte filenames (Microsoft does not delete own creature as malware)
+  ftp8bit.exe - MS Visual C++ 2010, uses wsock.lib old sockets library, only 8-bit filenames. Maybe some Win98 PCs could require.

Really tested. On an old Win95, set Control Panel>Networking>protocols Add TCP/IP, then set manually IP and subnet, then
open Windows Explorer to local address of PC running www_x64.exe, download ftp95.exe and start it. The listing of all files
can be created by list95.exe, which is similar to dir *.* /s to file.
Makes an FTP tunnel from a modern PC to the old PC, can do Windows ftp.exe 

Initially made for TinyC, the best compiler for very old Windows. Later added MS Visual C++ 2010 builds for Windows XP cases.
If Visual C++ version works, then use it. Read down about Security! This thing goes online.
The Win95 and 8-bit versions have no security permissions checks.

ftp_xp.exe server can serve one single connection.
No userlist, no ssl. No threading, minimal functionality.
Intended to send, receive and close the application.

It can serve Windows cmd.exe > ftp.exe
Do not forget set "binary" after login!!!
Or can use other ftp client apps.


Features:

Passive or port, active or binary data sending.
Can set UTF8 if filenames are encoded.
Encoded paths and files can be accessed by using "?" instead
of non-ascii letters. ftp>cd "J??AA"
Also can try OPTS UTF8 OFF, or ON. Sometimes helps.
The command ALNA lists alternative names of files and folders.
It is specially made for opening old folders created by non-IT members :)
So, can simply
 ftp>literal cd {123}
 ftp>literal site chmod 777 {34}
Values by meanins are 777-normal, 444-readonly, 000-hidden
Renaming sets normal file permission attributes automatically, try first.

If filenames contain "?" in lists then it means files have been saved using
Windows 2-byte encoding, which is much different to the ascii table.
This tool is 8-bit and maybe not the best choice then. If such named files too many.
During times people spent much resources developing encoding conversion rich user interfaces.
There were plenty of functions (ending W as SetCurrentDirectoryW) developed later
which should be used. But normally ascii was used naming files.
Some language encodings, as latin, are close to ascii, but some really technically encoded.
 
Ok, ok. I added a special case to rename some files and folders this way:
ALNA command displays list of files.
Instead of a filename the code {Nr} can be used. It is the key for a file the program finds.
ALNW command displays almost the same list with the code {wNr}. Use this(!) and rename
 abrakadabras by using the most core rename function for Windows 2-byte char.filenames.
After the file or folder has a good name to operate normally.
At least, this can be a solution for encodings. Of course, if such files are few.

Usage:

Place it in a folder and run.
The current folder of the application becomes the root "/"
Put and get, and rename, and delete too.
Limited to the root. Normally, but admin can access everything.

In fact, it works well on the local network.
The router and firewall also block ftp from outside.
 
Advice:

Nowadays just use a web-browser and download the needed things.
The www_xp.exe, for example, can do the same.

Anyway, better get a zipper(!) on that PC, zip the content with all the
encodings and get a single compressed file named "a.zip", and then
send with ftp.exe, or download from elsewhere. No need to look at each file remotely
through old interface tools. Of course, it depends on disk space capabilities. 

FTP is a very old thing. It becomes obsolete. Use curl or wget.
Or ssl based servers with multithreading and proper user management.
At least, they are known to security systems.

Security:

 This tool, in result, is too powerful, so had to make some restrictions.
Command line options define user access and permissions.
Help is on /? or /h.
Otherwise can get a responding something. Read the help and everything is here.
It is for people having good intentions. Tool can be evil we don't wanna be.

Issues observed:

Empty folders sometimes give errors on Windows Explorer.
I suspect Microsoft did not like ftp and made a garbage just to be.
These windows of folders mostly throw errors without explanation and error correction.
After they disabled ftp everywhere, obviously not to confuse users.

Important: always set binary file transfer, otherwise will get
truncated short files. ASCII was used on mainframes.
For windows ftp.exe it is "binary" command.

For encoded folders and filenames in ftp.exe use ""
cd "filename%(my)+-folder"

File sizes may be small because it isn't important in sending.
Ftp clients are made according to standards of 70s.

Modern Windows suspect TinyC executables as malwares and deletes them without a notice. 

Prior to it there were times of DOS Win3.1 Trumpet over telephone line and WS_ftp tool,
as I remember, from the tucows site once. Or others.
Nobody knew about ssl and harmful bots all day checking network sockets, by the way.

-----------------
Chessforeva 30%, ChatGPT 60%, Gemini 10%
2025
 