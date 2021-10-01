/* replace.h - All the boring definitions for replace.c
   (C) Richard K. Lloyd 2001-2004 */

/* Just reminding you what version we're up to... */
#define REPLACE_VERSION "2.24"
#define REPLACE_YEAR "2004"

/* Standard system header files */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <ftw.h>
#include <utime.h>
#include <signal.h>

/* Linux prefers to define getopt() in getopt.h */
#ifdef __linux__
#include <getopt.h>
#endif

/* Macros to allow prototypes to be declared when using
   ANSI C. Does nothing for K&R compilers */
#ifdef __STDC__
#define P(a) a
#define P2(a,b) a,b
#define P3(a,b,c) a,b,c
#else
#define P(a)
#define P2(a,b)
#define P3(a,b,c)
#endif

/* If compiled with -DHAVE_LONG_LONG, set up needed long long definitions */
#ifdef HAVE_LONG_LONG
#define LONG_LONG signed long long
#define LONG_LONG_FORMAT "%lld"
#else
#define LONG_LONG signed long
#define LONG_LONG_FORMAT "%ld"
#endif

/* No agreement as to malloc() types...grrr... */
#define MALLOCPTR void *
#define MALLOCPAR size_t

extern char *optarg;
extern int optind;

#define BIN_CHUNK 16384 /* 16K chunks of binary data */

#define MAXSTRS 255 /* Maximum number of replacement pairs */

#define MAX_DIR_LEVELS 32 /* Maximum dir recursion */

#define MAX_SOFT_LINKS 16 /* Maximum number of soft-links before
                             assume a soft-link loop */

#define MAX_BIN_BYTES 256 /* Examine these number of bytes for binary chars */

/* Just in case someone fools around with the values... */
#if MAX_BIN_BYTES > BIN_CHUNK
 #error MAX_BIN_BYTES must be <= BIN_CHUNK
#endif

#define DEF_BACKUP_SUFFIX ".cln" /* Default backup suffix */

#define DEF_TMP_DIR "/tmp" /* Default temporary directory */

#define TERMINAL_DEV "/dev/tty" /* Terminal device (keyboard) */

#define RENAME_COMMAND "/bin/mv -f" /* System command to rename files */

/* Global variables (define "Extern" as either nothing or extern) */

Extern char *oldstr[MAXSTRS+1]; /* Set of original strings */
Extern char *newstr[MAXSTRS+1]; /* Set of new strings */
Extern char *sufflist[MAXSTRS+1]; /* List of suffixes (-x option) */

/* Dynamically allocated buffer variables */
Extern char *binchunkptr; /* Binary replacement buffer */
Extern size_t binchunksize; /* Binary replacement buffer size */
Extern char *thestring; /* Text replacement buffer */
Extern size_t thestringsize; /* Text replacement buffer size */
Extern char *newline; /* New line output buffer */
Extern size_t newlinesize; /* New line output buffer size */

Extern char intitle[BUFSIZ]; /* Current file being string-replaced */
Extern char progname[BUFSIZ]; /* Basename of this executable */
Extern char backupsuff[BUFSIZ]; /* Backup suffix (usually .cln) */
Extern char filetype[16]; /* Holds "binary file" or "text file" depending on -b flag */
Extern char tmpdir[BUFSIZ]; /* Temporary direcotry to hold replaced file */
Extern char tempfile_cleanup[BUFSIZ]; /* Current temporary file */

Extern size_t oldstrlen[MAXSTRS],newstrlen[MAXSTRS]; /* Set of org/new strlens */

/* Misc. global int/size_t variables (see init.c) */
Extern int sensitive,word,force,prepadit,padit,verbose,startline,
           recursive,prompt,fake,numfilereps,numfiles,
           numreps,updated,linenum,repcount,binary,hex,maxreplines,
           maxtimes,zeroterm,linecount,retainstamp,followsoftlinks,ascii,
           autodetect;
Extern size_t autobinread,autobinsize,minsuff,numstrs,startcol,suffixes,
              sufflen;

/* Initialisation routines */
Extern void leave(P(char *)),
            tidy_up(P(void));

/* Other cross-source routines */
Extern void plural(P2(char *,LONG_LONG));
Extern int ask_user(P2(int,char *));
Extern char *alloc_mem(P3(char *,size_t *,size_t));
Extern char *basename_path(P(char *));
Extern void set_temp_file(P(char *));
