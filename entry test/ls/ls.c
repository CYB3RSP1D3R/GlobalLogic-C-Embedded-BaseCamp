
#include <stdio.h>    
#include <dirent.h>     // required for working with directories
#include <unistd.h>     // contains getcwd
#include <stdlib.h>
#include <sys/stat.h>   // required for information about files and folders
#include <errno.h>
#include <pwd.h>        // needs for userid->username convertation
#include <grp.h>        // needs for groupid->groupname convertation
#include <time.h>
#include <string.h> 

struct flags {
        unsigned int allfiles : 1; // shows hidden files, correspond to the flag -a
        unsigned int fullinfo : 1; // shows full information about the files, correspond to the flag -l
        unsigned int specpath : 1; // flag if filepath is specified during the call 
} flags;

struct filedata {
        struct dirent *diritem;
        struct stat stats;
        char *username;
        char *groupname;
        int filesize;           // if a file is a directory and prevent the same calculations
};

/*
 * this function is passed to qsort as a function of comparison 2 structs
 * uses strcmp for comparision 
 */
int fdcomp(const void *a, const void *b);

/*
 * returns the number of the items in a directory
 * after job is done directory stream is reset to the beginning
 */
int dirlen(DIR *dirp);

/*
 * returns number characters required for int number
 */
int deccharnum(int number);

/*
 * recursively calculate the size of all files and nested directories inside except hidden files
 * return -1 if input argument is not a directory 
 */
int sizeofdir(char *filepath);

/* decode fs.st_stat to rwx message for ls -l
 * as it is used only in this programm this function does not check the input arguments
 */
char *modedcd(struct stat fs, char dest[]);

int main(int argc, char **argv)
{
        char *dirname = NULL;
        DIR *dirp = NULL;
        struct filedata *dirlist;
        int totalblocks = 0;
        int dlen;
        int spaces[4] = {0, 0, 0, 0}; // max spaces for nlinks, owner, group, size
        char rwx [11];
        char strtm [13];
        struct tm *modtime;
        int tmp; // just for a comfort, compiler may ignore it

        // in initial state all flags are set in zero
        flags.specpath = 0;
        flags.allfiles = 0;
        flags.fullinfo = 0;

        // filepath is specified in the last argument 
        if (argv[argc-1][0] == '/') {
                dirname = argv[--argc];
                flags.specpath = 1;
        }

        // arguments reading
        while (--argc != 0) {
                if (argv[argc][0] == '-') {
                        int len = strlen(argv[argc]);
                        for (int i = 1; i < len; i++) {
                                switch (argv[argc][i])
                                {
                                case 'a':
                                        flags.allfiles = 1;
                                        break;
                                case 'l':
                                        flags.fullinfo = 1;
                                        break;
                                default:
                                        break;
                                }
                        }
                } else {
                        printf("Incorrect giving of the agruments\n");
                        return 1;
                }
        }

        // if full filepath is not specified the default one is used
        // default filepath is current working directory
        if (!dirname) { 
                dirname = getcwd(NULL, 0);
        }

        dirp = opendir(dirname);

        if (!dirp) {
                perror("Error reading the directory");
                return errno;
        }

        dlen = dirlen(dirp);
        dirlist = malloc(sizeof(*dirlist)*dlen);
        if (!dirlist) {
                perror("Error with allocating the memory");
                return errno;
        }

        for (int i = 0; i < dlen; i++) {
                dirlist[i].diritem = readdir(dirp);
                if (stat(dirlist[i].diritem->d_name, &dirlist[i].stats)) {
                        perror("Error called by 'stat'");
                        return errno;
                }
                dirlist[i].username = (getpwuid(dirlist[i].stats.st_uid))->pw_name;
                dirlist[i].groupname = (getgrgid(dirlist[i].stats.st_gid))->gr_name;
                if (S_ISDIR(dirlist[i].stats.st_mode) && (dirlist[i].diritem->d_name)[0] != '.') { // hidden folders/files could be opened only with sudo otherwise it causes the error
                        dirlist[i].filesize = sizeofdir(dirlist[i].diritem->d_name);
                } else {
                        dirlist[i].filesize = dirlist[i].stats.st_size;
                }

                if (flags.allfiles == 1 
                    || (dirlist[i].diritem->d_name)[0] != '.'){
                        totalblocks += dirlist[i].stats.st_blocks; //Here blocks have a size = 512 bytes
                }

                tmp = deccharnum(dirlist[i].stats.st_nlink);
                spaces[0] = (tmp > spaces[0] ? tmp : spaces[0]);
                tmp = strlen(dirlist[i].username);
                spaces[1] = (tmp > spaces[1] ? tmp : spaces[1]);
                tmp = strlen(dirlist[i].groupname);
                spaces[2] = (tmp > spaces[2] ? tmp : spaces[2]);
                tmp = deccharnum(dirlist[i].filesize);
                spaces[3] = (tmp > spaces[3] ? tmp : spaces[3]);
        }

        qsort(dirlist, dlen, sizeof(*dirlist), fdcomp);
        
        if (flags.fullinfo == 1) {
                printf("total %li\n", totalblocks/2); // print with a size of block = 1024 bytes
        }
        
        if (flags.fullinfo == 1) {
                for (int i = 0; i < dlen; i++) {
                        if (flags.allfiles == 1 || (dirlist[i].diritem->d_name)[0] != '.') {
                                modtime = localtime(&(dirlist[i].stats.st_mtime));
                                strftime(strtm, 13, "%b %d %R", modtime);
                                printf("%s %*d %*s %*s %*li %s %s\n", 
                                        modedcd(dirlist[i].stats, rwx), spaces[0], dirlist[i].stats.st_nlink, 
                                        spaces[1], dirlist[i].username, spaces[2], dirlist[i].groupname, 
                                        spaces[3], dirlist[i].filesize, strtm, dirlist[i].diritem->d_name);

                        }
                }
        } else {
                for (int i = 0; i < dlen; i++) {
                        if (flags.allfiles == 1 || (dirlist[i].diritem->d_name)[0] != '.') {
                                printf("%s\t", (dirlist[i].diritem->d_name));
                        }
                }
                printf("\n");
                        
        }

        closedir(dirp);

        // output of getcwd is in the dynamically allocated memory so it should be free
        if (flags.specpath == 0) {
                free(dirname);
        }
        free(dirlist);

        return 0;
}

int fdcomp(const void *a, const void *b)
{
        const struct filedata *fda = a;
        const struct filedata *fdb = b;

        return strcasecmp(fda->diritem->d_name, fdb->diritem->d_name);
}

int dirlen(DIR *dirp)
{
        int len = 0;
        struct dirent *entry;

        rewinddir(dirp); // in case if directory had been read before
        while ((entry = readdir(dirp)) != NULL) {
                len++;
        }
        rewinddir(dirp);

        return len;
}

int deccharnum(int number) 
{
        int ndigits = (number < 0 ? 1 : 0);

        while (number != 0) {
                ndigits++;
                number /= 10;
        }
        return ndigits;
}

int sizeofdir(char *filepath)
{       
        int dirsize = 0;
        struct dirent *entry;
        struct stat fs;

        DIR *dirp = opendir(filepath);
        if (!dirp) {
                return -1;
        }
        chdir(filepath); // required for reading the files inside the folder

        while ((entry = readdir(dirp)) != NULL) {
                if (entry->d_name[0] != '.') {
                        stat(entry->d_name, &fs);
                        if (S_ISDIR(fs.st_mode)) {
                                dirsize += sizeofdir(entry->d_name);
                        } else {
                                dirsize += fs.st_size;
                        }
                } else {
                        dirsize += 4096; // the minimum size a file or directory entry/link
                }
        }

        closedir(dirp);
        chdir(".."); // return to the previous directory
        return dirsize + 4096; // calculated size plus size of a directory entry
}

char *modedcd(struct stat fs, char *dest)
{
        // decode file type
        if (S_ISREG(fs.st_mode)) {
                dest[0] = '-';
        } else if (S_ISBLK(fs.st_mode)) {
                dest[0] = 'b';
        } else if (S_ISCHR(fs.st_mode)) {
                dest[0] = 'c';
        } else if (S_ISDIR(fs.st_mode)) {
                dest[0] = 'd';
        } else if (S_ISLNK(fs.st_mode)) {
                dest[0] = 'l';
        } else if (S_ISFIFO(fs.st_mode)) {
                dest[0] = 'p';
        } else if (S_ISSOCK(fs.st_mode)) {
                dest[0] = 's';
        }

        // owner`s read/write/execute permissions
        if (S_IRUSR & fs.st_mode) {
                dest[1] = 'r';
        } else {
                dest[1] = '-';
        }

        if (S_IWUSR & fs.st_mode) {
                dest[2] = 'w';
        } else {
                dest[2] = '-';
        }

        if (S_IXUSR & fs.st_mode) {
                dest[3] = 'x';
        } else {
                dest[3] = '-';
        }

        // group`s read/write/execute permissions
        if (S_IRGRP & fs.st_mode) {
                dest[4] = 'r';
        } else {
                dest[4] = '-';
        }

        if (S_IWGRP & fs.st_mode) {
                dest[5] = 'w';
        } else {
                dest[5] = '-';
        }

        if (S_IXGRP & fs.st_mode) {
                dest[6] = 'x';
        } else {
                dest[6] = '-';
        }

        // other`s read/write/execute permissions
        if (S_IROTH & fs.st_mode) {
                dest[7] = 'r';
        } else {
                dest[7] = '-';
        }

        if (S_IWOTH & fs.st_mode) {
                dest[8] = 'w';
        } else {
                dest[8] = '-';
        }

        if (S_IXOTH & fs.st_mode) {
                dest[9] = 'x';
        } else {
                dest[9] = '-';
        }

        dest[10] = '\0';

        return dest;
}

