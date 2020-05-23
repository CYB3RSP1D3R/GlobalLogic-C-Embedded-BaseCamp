
This program is an analog for the ls console command. It is made for GlobalLogic C/Embedded BaseCamp Entry Test.

It supports 2 flags:
    -l -- print full information.
    -a -- print all files.
    
Separate flags indication is supported. 
The last argument is a file path. If the file path is not indicated, current working directory`s is used instead.
Output of the program is sorted by the file`s name. 

Full information format:
    * 10 characters:
        * 1st character indicates a type of the file:
            * - -- normal file.
            * b -- block device.
            * c -- character device.
            * d -- directory.
            * l -- symbolic link.
            * p -- named pipe.
            * s -- socket.
        * next 9 characters indicate the permissions:
            * r -- read.
            * w -- write.
            * x -- execute.
        First 3 for the owner. Second 3 for the group. Last 3 for other users.
    * Number of links.
    * Owner.
    * Group.
    * Size in bytes.
    * Last modified date and time.
    * File`s name.

*Total is the number of the total used blocks, size of a block = 1024 bytes.

This format correspond to ls`s one. 
Size of folder is calculated as the summary size of all files inside this folder. For nested folder`s the size is calculated recursively.
