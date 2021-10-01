/* int.c - Initialisation code for replace command
   (C) Richard K. Lloyd 2001-2004
*/

#define Extern
#include "replace.h"

/* This next string gets output when you do a "what replace" in HP-UX */
static char ident[] = "@(#)replace " REPLACE_VERSION " (C) Richard K. Lloyd " REPLACE_YEAR;

#ifdef __STDC__
void set_temp_file(char *tfile)
#else
void set_temp_file(tfile)
char *tfile;
#endif
{
   /* The file passed to this routine should be unlinked in the tidy_up()
      routine. Passing "" as a file clears this and won't unlink during
      tidy_up(); */
   (void)strcpy(tempfile_cleanup,tfile);
}

void tidy_up(P(void))
{
   /* I don't actually have to include this routine since UNIX does it for me
      when the program exits, but let's be careful anyway - you never know,
      it might be a library one day :-) */

   size_t f;

   for (f=1;f<=numstrs;f++)
   {
      if (oldstr[f]!=(char *)NULL) free((MALLOCPTR)oldstr[f]);
      if (newstr[f]!=(char *)NULL) free((MALLOCPTR)newstr[f]);
   }
   for (f=1;f<=suffixes;f++)
   if (sufflist[f]!=(char *)NULL) free((MALLOCPTR)sufflist[f]);
   if (binchunkptr!=(char *)NULL) free((MALLOCPTR)binchunkptr);
   if (thestring!=(char *)NULL) free((MALLOCPTR)thestring);

   /* Remove any temp file that was being worked on at the time */
   if (tempfile_cleanup[0]) (void)unlink(tempfile_cleanup);
}

#ifdef __STDC__
void tidy_up_handler(int signum)
#else
void tidy_up_handler(signum)
int signum;
#endif
{
   /* Handle signal appropriately and then exit the program */
   (void)signal(signum,SIG_DFL); /* Reset handler back to default */
   tidy_up();
   exit(EXIT_FAILURE);
}

void init_vars(P(void))
{
   /* Initialise global variables */

   set_temp_file(""); /* No temporary file to clean up yet */
   numstrs=0; /* No strings yet */
   numfilereps=0; /* Number of files that had their strings replaced */
   numfiles=0; /* Total number of files scanned */
   numreps=0; /* Total number of replacements */

   /* Default settings if no qualifiers supplied */
   sensitive=0; /* Case-insensitive */
   word=1; /* Match words only, not sub-strings */
   force=0; /* Don't force it - keep .cln version */
   padit=0; /* Don't pad new string with trailing spaces */
   prepadit=0; /* Don't pad new string with leading spaces */
   verbose=0; /* Not verbose */
   startcol=1; /* Start replacing from column 1 */
   startline=1; /* Start replacing from line 1 */
   recursive=0; /* Don't recurse */
   suffixes=0; /* No suffixes yet */
   minsuff=BUFSIZ; /* Minimum length of a suffix */
   prompt=0; /* Don't prompt yet */
   fake=0; /* Don't "fake" the replacements */
   binary=0; /* Not forcing binary replacements */
   ascii=0; /* Not forcing text replacements */
   hex=0; /* Not hex strings */
   maxreplines=0; /* Replace in any number of lines */
   maxtimes=0; /* Replace string any number of times in a single line */
   zeroterm=0; /* Don't zero-terminate */
   binchunkptr=(char *)NULL; binchunksize=0; /* No binary chunk yet */
   thestring=(char *)NULL; thestringsize=0; /* No text string yet */
   newline=(char *)NULL; newlinesize=0; /* No newstr output buffer yet */
   retainstamp=0; /* Don't keep the timestamp of the original file */
   followsoftlinks=0; /* Ignore soft-links if specified as filenames */
   autodetect=1; /* Auto-detect if binary or text */
   autobinsize=0; /* Didn't read any bytes for auto-detect yet */

   /* Now set up signals to run the tidy_up_handler() routine */
   (void)signal(SIGINT,tidy_up_handler);
}

#ifdef __STDC__
void leave(char *errmess)
#else
void leave(errmess)
char *errmess;
#endif
{
   /* Something wrong, so leave, displaying an error message if non-null */
   tidy_up();
   if (errmess[0]) (void)fprintf(stderr,"%s: FATAL - %s\n",progname,errmess);
   exit(EXIT_FAILURE);
}

#ifdef __STDC__
static void add_suffix(char *sfx)
#else
static void add_suffix(sfx)
char *sfx;
#endif
{
   /* New suffix to add to the list */
   if (suffixes<MAXSTRS)
   {
      size_t sxlen=strlen(sfx);
      if (sxlen<minsuff) minsuff=sxlen;
      if ((sufflist[++suffixes]=(char *)malloc(sxlen+1))==(char *)NULL)
         leave("Can't allocate memory for new suffix");
      (void)strcpy(sufflist[suffixes],sfx);
   }
   else
   (void)fprintf(stderr,"WARNING: Already at limit of %d suffixes - ignored %s suffix\n",MAXSTRS,sfx);
}

#ifdef __STDC__
static void padline(char *pstr)
#else
static void padline(pstr)
char *pstr;
#endif
{
   /* Pad syntax options to line up after the "Syntax: " string */
   (void)fprintf(stderr,"%*s%s\n",(int)strlen(progname)+9,"",pstr);
}

static void usage(P(void))
/* Tell the user what to type next time... */
{
   (void)fprintf(stderr,"%s - The sane person's alternative to sed\n\n",&ident[4]);
   (void)fprintf(stderr,"Syntax: %s [-?] [-A] [-b] [-c startcol] [-d tempdir] [-e] [-f]\n",progname);
   padline("[-h] [-i] [-L] [-l linenum] [-m maxreplines] [-n] [-P] [-p]");
   padline("[-r|R] [-s] [-T] [-t maxtimes] [-u backupsuffix] [-v]");
   padline("[-w] [-x suffix [-x...]] [-z]");
   padline("old_str new_str [-a old_str new_str...] [filename...]\n");
   (void)fprintf(stderr,"-? displays this syntax message.\n");
   (void)fprintf(stderr,"-A forces the program to assume all files are text files. Normally, the first\n");
   (void)fprintf(stderr,"   %d bytes are examined to auto-determine if a file is text or binary.\n",MAX_BIN_BYTES);
   (void)fprintf(stderr,"   This option is deprecated and may be changed or removed in a future release.\n");
   (void)fprintf(stderr,"-a allows extra pairs of strings to be replaced in order.\n");
   (void)fprintf(stderr,"-b forces the program to assume all files are binary files.\n");
   (void)fprintf(stderr,"-c startcol starts the string replace from column 'startcol' rather than the\n");
   (void)fprintf(stderr,"   first column.\n");
   (void)fprintf(stderr,"-d specifies the temporary directory for storing modified files. If not\n");
   (void)fprintf(stderr,"   supplied, the default directory is $TMPDIR or, if that isn't set, %s.\n",DEF_TMP_DIR);
   (void)fprintf(stderr,"-e makes search case-sensitive. new_str exactly replaces old_str\n");
   (void)fprintf(stderr,"   with no case-adjustment to new_str.\n");
   (void)fprintf(stderr,"-f forces the update of files without any %s backup.\n",backupsuff);
   (void)fprintf(stderr,"-h indicates that replacement pairs are binary hex data.\n");
   (void)fprintf(stderr,"-i interactively prompts the user to confirm if they want strings replaced in\n");
   (void)fprintf(stderr,"   next file. Specifying -i -i switches to prompting for each replacement.\n");
   (void)fprintf(stderr,"-L follows soft-links specified on the command line.\n");
   (void)fprintf(stderr,"-l linenum starts the string replacement from line 'linenum' rather than the\n");
   (void)fprintf(stderr,"   first line.\n");
   (void)fprintf(stderr,"-m maxreplines specifies the maximum number of lines in a file that should\n");
   (void)fprintf(stderr,"   have string replacements.\n");
   (void)fprintf(stderr,"-n displays what strings would be replaced without actually replacing them.\n");
   (void)fprintf(stderr,"   It also switches on verbose mode.\n");
   (void)fprintf(stderr,"-P pre-pads new_str with leading spaces if it is shorter than old_str.\n");
   (void)fprintf(stderr,"-p pads new_str with trailing spaces if it is shorter than old_str.\n");
   (void)fprintf(stderr,"-r or -R recurses down the current directory tree (if no filenames are given),\n");
   (void)fprintf(stderr,"   replacing strings in all files found. Use -x to narrow the recursion.\n");
   (void)fprintf(stderr,"-s relaxes the condition that old_str has to match a 'word' i.e. it replaces\n");
   (void)fprintf(stderr,"   old_str even if it is a substring of a 'word'.\n");
   (void)fprintf(stderr,"-T retains the timestamps of the original unmodified files.\n");
   (void)fprintf(stderr,"-t maxtimes states the maximum number of times a string can be replaced\n");
   (void)fprintf(stderr,"   in any single line of a file.\n");
   (void)fprintf(stderr,"-u backupsuffix specifies the backup suffix for the unmodified file.\n");
   (void)fprintf(stderr,"-v increments (switches on) verbose mode, reporting a summary of replacements\n");
   (void)fprintf(stderr,"   if specified once and all replacements if specified twice (i.e. -vv).\n");
   (void)fprintf(stderr,"-w recursively replaces strings in all Web-related source files in the current\n");
   (void)fprintf(stderr,"   directory tree. Equivalent to \"-r -x .html -x .htm -x .asp -x .js -x .css\n");
   (void)fprintf(stderr,"   -x .xml -x .xhtml -x .shtml -x .jsp -x .php -x .php3 -x .php4 -x .pl\".\n");
   (void)fprintf(stderr,"-x suffix specifies a case-insensitive filename suffix to look for when\n");
   (void)fprintf(stderr,"   recursing. Multiple -x params can be supplied and have an \"or\" meaning.\n");
   (void)fprintf(stderr,"-z Zero-terminate any binary replacement string.\n");
   leave("");
}

#ifdef __STDC__
void get_options(int argc,char **argv)
#else
void get_options(argc,argv)
int argc;
char **argv;
#endif
/* Parse the standard qualifiers */
{
   int param,looping=1;
   size_t alloclen,newtmp,oldtmp;
   char *tmpdirptr=getenv("TMPDIR");

   (void)strcpy(progname,basename_path(argv[0])); /* Get `basename $0` */
   (void)strcpy(backupsuff,DEF_BACKUP_SUFFIX);
   if (tmpdirptr==(char *)NULL)
      (void)strcpy(tmpdir,DEF_TMP_DIR);
   else (void)strcpy(tmpdir,tmpdirptr);

   while ((param = getopt(argc,argv,"+Abc:d:efhiLl:m:nPprRsTt:u:vwx:z?")) != EOF)
   {
      switch (param)
      {
         case '?': usage(); /* Bad option */
                   break;
         case 'A': ascii=1;
                   (void)fprintf(stderr,"WARNING: The current behaviour of -A is deprecated - this option may change\n");
                   (void)fprintf(stderr,"         its functionality or be removed in a future release.\n");
                   break;
         case 'b': binary=1;
                   break;
         case 'c': startcol=(size_t)atoi(optarg);
                   break;
         case 'd': (void)strcpy(tmpdir,optarg);
                   break;
         case 'e': sensitive=1;
                   break;
         case 'f': force=1;
                   break;
         case 'h': hex=1; binary=1; sensitive=1;
                   break;
         case 'i': if (prompt<2) prompt++;
                   break;
         case 'L': followsoftlinks=1;
                   break;
         case 'l': startline=atoi(optarg);
                   break;
         case 'm': maxreplines=atoi(optarg);
                   break;
         case 'n': if (fake<2) fake++;
                   break;
         case 'P': prepadit=1;
                   break;
         case 'p': padit=1;
                   break;
         case 'R':
         case 'r': recursive=1;
                   break;
         case 's': word=0;
                   break;
         case 'T': retainstamp=1;
                   break;
         case 't': maxtimes=atoi(optarg);
                   break;
         case 'u': (void)strcpy(backupsuff,optarg);
                   break;
         case 'v': if (verbose<2) verbose++;
                   break;
         case 'w': recursive=1;
                   add_suffix(".html");
                   add_suffix(".htm");
                   add_suffix(".asp");
                   add_suffix(".js");
                   add_suffix(".css");
                   add_suffix(".xml");
                   add_suffix(".xhtml");
                   add_suffix(".shtml");
                   add_suffix(".jsp");
                   add_suffix(".php");
                   add_suffix(".php3");
                   add_suffix(".php4");
                   add_suffix(".pl");
                   break;
         case 'x': add_suffix(optarg);
                   break;
         case 'z': zeroterm=1;
                   break;
      }
   }

   /* Adjust verbosity level */
   if (verbose<prompt) verbose=prompt;
   if (verbose<fake) verbose=fake;

   if (padit && prepadit) leave("You can't specify both -P and -p: they are mutually exclusive");

   if (ascii && binary) leave("You can't specify both -A and -b: they are mutally exclusive");
   if (ascii || binary) autodetect=0;

   if (binary)
   {
      if (hex || padit || prepadit) zeroterm=0;
      startcol=1; startline=1;
      maxreplines=0; word=0;
   }
   else
   if (zeroterm) leave("You must additionally specify -b when using -z");

   sufflen=strlen(backupsuff);

   while (looping)
   {
      if (optind+2>argc) usage();

      oldtmp=strlen(argv[optind]);
      if (!oldtmp && binary) leave("Old string must be non-null");

      if (hex)
      {
         int hptr=2;
         size_t hlp;
         int hval;

         if (oldtmp%2) hptr--;
         oldtmp=(oldtmp+1)/2;
         if ((oldstr[++numstrs]=(char *)malloc((MALLOCPAR)oldtmp))==(char *)NULL)
            leave("Can't allocate memory for old hex string");
         for (hlp=0;hlp<oldtmp;hlp++)
         {
            char oarg=argv[optind][hptr];
            argv[optind][hptr]='\0';
            hval=0;
            (void)sscanf(&argv[optind][hptr-1-(hptr>1)],"%x",&hval);
            oldstr[numstrs][hlp]=(char)hval;
            argv[optind][hptr]=oarg;
            hptr+=2;
         }
      }
      else
      {
         if ((oldstr[++numstrs]=(char *)malloc((MALLOCPAR)(oldtmp+1)))==(char *)NULL)
            leave("Can't allocate memory for old string");
         (void)strcpy(oldstr[numstrs],argv[optind]);
      }
      oldstrlen[numstrs]=oldtmp;

      newtmp=strlen(argv[++optind]);
      if (!newtmp && binary) leave("New string must be non-null");

      if ((prepadit || padit) && newtmp<oldtmp && !hex) alloclen=oldtmp; else alloclen=newtmp;

      if (hex)
      {
         int hptr=2;
         int hval;
         size_t hlp;

         if (newtmp%2) hptr--;
         newtmp=(newtmp+1)/2;
         alloclen=newtmp;
         if ((newstr[numstrs]=(char *)malloc((MALLOCPAR)alloclen))==(char *)NULL)
            leave("Can't allocate memory for new hex string");
         for (hlp=0;hlp<newtmp;hlp++)
         {
            char oarg=argv[optind][hptr];
            argv[optind][hptr]='\0';
            hval=0;
            (void)sscanf(&argv[optind][hptr-1-(hptr>1)],"%x",&hval);
            newstr[numstrs][hlp]=(char)hval;
            argv[optind][hptr]=oarg;
            hptr+=2;
         }
      }
      else
      {
         if ((newstr[numstrs]=(char *)malloc((MALLOCPAR)(alloclen+1)))==(char *)NULL)
            leave("Can't allocate memory for new string");
         (void)strcpy(newstr[numstrs],argv[optind]);
         if (padit)
         {
            while (newtmp<oldtmp) { newstr[numstrs][newtmp++]=' '; }
            newstr[numstrs][newtmp]='\0';
         }
         else
         if (prepadit && newtmp<oldtmp)
         {
            size_t cp,sdf=oldtmp-newtmp;
            for (cp=oldtmp-1;cp>=sdf;cp--)
            { newstr[numstrs][cp]=newstr[numstrs][cp-sdf]; }
            for (cp=0;cp<sdf;cp++) newstr[numstrs][cp]=' ';
            newstr[numstrs][newtmp=oldtmp]='\0';
         }
      }
      newstrlen[numstrs]=alloclen+zeroterm;

      if (binary && newtmp>oldtmp) leave("New binary string mustn't be larger than old binary string");

      if (zeroterm && newtmp>=oldtmp) leave("New binary string must be smaller than old binary string if -z used");

      if (hex)
      {
         if (newtmp==oldtmp && !memcmp((void *)oldstr[numstrs],(void *)newstr[numstrs],oldtmp))
            leave("Old and new hex strings must be different");
      }
      else
      if ((sensitive && !strcmp(oldstr[numstrs],newstr[numstrs])) ||
           (!sensitive && !strcasecmp(oldstr[numstrs],newstr[numstrs])))
         leave("Old and new strings must be different");

      /* Yes, I could short-cut this, but some lints complain if I do */
      looping=(++optind<argc);
      if (looping)
      {
         looping=(!strcmp(argv[optind],"-a") && numstrs<MAXSTRS);
         if (looping) optind++;
      }
   }

   if (suffixes && !recursive) leave("You must use -x in conjunction with -r or -w");

   if (verbose && !binary)
      (void)fprintf(stderr,"Text file replacements start from column %d on each line and from line %d onwards\n",(int)startcol,startline);
   startcol--; /* Adjust start column to match char array index */
}
