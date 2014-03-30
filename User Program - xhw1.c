/*
* hw1-llagudu/hw1/xhw1.c
*
* Userland program to invoke the system call
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#define __NR_xconcat    349     /* our private syscall number */
#define __user

/*
* Declaration of struct variable which stores the
* command line arguments specified by the user
*/
struct myargs {
        __user const char *outfile;/* Name of the output file*/
        __user const char **infiles;/* Array with names of input file */
        unsigned int infile_count;/* Number of input files in infiles array */
        int oflags;/* Open flags to change behavior of syscall */
        mode_t mode;/* Default permission mode for newly created outfile */
        unsigned int flags;/* Special flags to change behavior of syscall */
};
/*
* main    - Entry point of the program
* @argc:    Number of arguments entered in the command line
* @argv:    Pointer array which contains the command line arguments
*/

int main(int argc, char *argv[])
{
        int rc = 0, c = 0, i = 0, j = 0;
        int oflags1 = O_RDWR;
        mode_t mode1 = 0;
        int flags1 =  0;
        char *string = malloc(sizeof(char *));
        char *cMode = NULL;
	struct myargs args;
        while ((c = getopt(argc, argv, "acteANPm:h")) != -1)
                switch (c) {
                case 'a':
                        oflags1 = oflags1 | O_APPEND;
                        break;
                case 'c':
                        oflags1 = oflags1 | O_CREAT;
                        break;
                case 't':
                        oflags1 = oflags1 | O_TRUNC;
                        break;
                case 'e':
                        oflags1 = oflags1 | O_EXCL;
                        break;
                case 'A':
                        flags1 = flags1 + 2;
                        break;
                case 'N':
                        flags1 = flags1 + 6;
                        break;
                case 'P':
                        flags1 = flags1 + 9;
                        break;
                case 'm':
                        mode1 = strtol(optarg, NULL, 8);
                        cMode = optarg;
                        break;
                case 'h':
                        fprintf(stderr, "Usage: %s [- acteANPmh] [-n] name\n",
                                argv[0]);
                        exit(EXIT_FAILURE);
                case '?':
                        fprintf(stderr, "Usage: %s [- acteANPmh] [-n] name\n",
                                argv[0]);
                        exit(EXIT_FAILURE);
                }

        if (oflags1 == O_RDWR)
                oflags1 = O_RDWR | O_APPEND;
        args.oflags = oflags1;
        args.mode = mode1;
	args.flags = flags1;
        args.outfile = malloc(sizeof(char));
        args.outfile = argv[optind];
        args.infile_count = argc - optind - 1;
        args.infiles = malloc((argc-optind-1) * sizeof(char *));

        for (i = optind+1, j = 0; i < argc ; i++, j++) {
                args.infiles[j] = malloc(sizeof(argv[i]) * sizeof(char));
                args.infiles[j] = argv[i];
        }

        if (cMode != NULL) {
                sprintf(string, "%o", mode1);
                if (strcmp(string, cMode) != 0) {
                        printf("\n Error: Invalid Mode\n");
                        free(string);
                        exit(0);
                }
        }
        void *dummy = (void *)&args;
        rc = syscall(__NR_xconcat, dummy, sizeof(struct myargs));

        if (rc >= 0) {
                if (flags1 == 6 || flags1 == 8)
                        printf("\n%d\n", rc);
                else if (flags1 == 9 || flags1 == 11)
                        printf("\n%d\n", rc);
                else
                        printf("\n%d\n", rc);
        }

        /* Print the appropriate error */
        else
                printf("\nErrorno: %d\t %s\n", errno, strerror(errno));

        exit(rc);
}
                  
