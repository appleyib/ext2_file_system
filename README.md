# ext2_file_system
A simple ext2 file system implmented by C

use make command to compile

All .img files are image files, all ext2 commands are implemented on those image files

Details for each command are following:

ext2_cp: This program takes three command line arguments. The first is the name of an ext2 formatted virtual disk. The second is the path to a file on your native operating system, and the third is an absolute path on your ext2 formatted disk. The program works like cp, copying the file on your native file system onto the specified location on the disk.

ext2_mkdir: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on your ext2 formatted disk. The program works like mkdir, creating the final directory on the specified path on the disk.

ext2_ln: This program takes three command line arguments. The first is the name of an ext2 formatted virtual disk. The other two are absolute paths on your ext2 formatted disk. The program works like ln, creating a link from the first specified file to the second specified path.

ext2_rm: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk, and the second is an absolute path to a file or link (not a directory) on that disk. The program works like rm, removing the specified file from the disk.

ext2_restore: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk, and the second is an absolute path to a file or link (not a directory!) on that disk. The program is the exact opposite of rm, restoring the specified file that has been previous removed.

ext2_checker: This program takes only one command line argument: the name of an ext2 formatted virtual disk. The program implements a lightweight file system checker, which detects a small subset of possible file system inconsistencies and takes appropriate actions to fix them.