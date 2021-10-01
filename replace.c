/* replace.c - The ideal program for sed-haters (i.e. everyone).
   (C) Richard K. Lloyd 2001-2004

   Provides a simple way of replacing strings in lines without resorting to
   the ludicrous gibberish of sed, whose syntax was obviously dreamt up by
   someone on LSD !

   Syntax:
   replace [qualifiers] <oldstr> <newstr> [-a <oldstr> <newstr> [-a...]]
           [filename...]

   What "replace" does by default:

   It performs a case-insensitive search for the "word" <oldstr> in each text
   line and replaces it with <newstr>, which is case-adjusted to match the
   case of the particular occurrence of <oldstr> in the original line. A "word"
   is defined to be a string that is preceded AND followed by a non-
   alphanumeric character (start and end of lines count as such a character)
   in the original text line. Multiple strings can be replaced in order using
   the "-a" qualifier.

   The qualifiers modify this behaviour appropriately (try "replace -?"). */
  
#define Extern extern
#include "replace.h"

/* Routines only called from this source file and nowhere else */
extern int binary_io(P2(FILE *,FILE *)),text_io(P2(FILE *,FILE *)),
           is_binary(P(FILE *));
extern void init_vars(P(void));
extern void get_options(P2(int,char **));

/* Need to track original pwd if following soft links */
static char curpwd[BUFSIZ];

#ifdef __STDC__
int ask_user(int pval,char *askname)
#else
int ask_user(pval,askname)
int pval;
char *askname;
#endif
{
   /* Ask the user if they want to replace a string (or strings) */
   int invalid=(prompt==pval);
   while (invalid)
   {
      FILE *thand;
      char tbuff[BUFSIZ]; tbuff[0]='\0';
      if ((thand=fopen(TERMINAL_DEV,"r"))==(FILE *)NULL)
         leave("Can't open " TERMINAL_DEV " - aborted");
      (void)fprintf(stderr,"Replace %s (y/n/a/q) ? [y] ",askname);
      if (fgets(tbuff,(int)BUFSIZ,thand)==(char *)NULL)
         leave("Can't read terminal input - aborted");
      (void)fclose(thand);
      switch (tbuff[0])
      {
         case 'Y':
         case 'y':
         case '\n':
           (void)fprintf(stderr,"OK, will replace %s\n",askname);
           invalid=0;
           break;
         case 'N':
         case 'n':
           (void)fprintf(stderr,"Skipped replace of %s\n",askname);
           return(0);
         case 'A':
         case 'a':
           (void)fprintf(stderr,"OK, prompting has been turned off.\n");
           (void)fprintf(stderr,"All appropriate strings will be replaced in all specified files\n");
           prompt=0;
           return(1);
         case 'Q':
         case 'q':
           leave("Program quit at user's request");
           break;
         default:
           (void)fprintf(stderr,"Invalid response - please try again\n");
           break;
      }

   }
   return(1);
}

#ifdef __STDC__
void plural(char *str,LONG_LONG value)
#else
void plural(str,value)
char *str;
LONG_LONG value;
#endif
{
   /* Nicely format a numeric message. Note the kludgy check for <10 here -
      this is so that the switch value can be cast down to a long safely.
      Yep, dumb HP-UX 11.00 HP ANSI C compiler refuses to switch on a
      long long value <rolls eyes upwards> */
   if (value<10)
   switch ((long)value)
   {
      case 0: (void)fprintf(stderr,"no %ss",str); break;
      case 1: (void)fprintf(stderr,"one %s",str); break;
      case 2: (void)fprintf(stderr,"two %ss",str); break;
      case 3: (void)fprintf(stderr,"three %ss",str); break;
      case 4: (void)fprintf(stderr,"four %ss",str); break;
      case 5: (void)fprintf(stderr,"five %ss",str); break;
      case 6: (void)fprintf(stderr,"six %ss",str); break;
      case 7: (void)fprintf(stderr,"seven %ss",str); break;
      case 8: (void)fprintf(stderr,"eight %ss",str); break;
      default: (void)fprintf(stderr,"nine %ss",str); break;
   } 
   else (void)fprintf(stderr,LONG_LONG_FORMAT " %ss",value,str);
}

#ifdef __STDC__
static int read_lines(FILE *fd,FILE *fdout,LONG_LONG filesize,char *filename)
#else
static int read_lines(fd,fdout,filesize,filename)
FILE *fd,*fdout;
LONG_LONG filesize;
char *filename;
#endif
{
   /* Handle a whole file. Returns 0 = success, != 0 for failure */
   int gotfail,this_binary=is_binary(fd);
   if (verbose==2)
   {
      (void)fprintf(stderr,"Scanning %s %s",filetype,filename);
      if (filesize>=0)
      {
         (void)fprintf(stderr," (");
         plural("byte",filesize);
         (void)fprintf(stderr,")");
      }
      (void)fprintf(stderr,"...\n");
   }
   linenum=0; repcount=0; numfiles++;
   if (this_binary) gotfail=binary_io(fd,fdout);
   else gotfail=text_io(fd,fdout);
   if (verbose && !gotfail)
   {
      if (verbose==2 || repcount)
      {
         (void)fprintf(stderr,"The %s %s ",filetype,intitle);
         if (!this_binary)
         {
            (void)fprintf(stderr,"contained ");
            plural("line",(LONG_LONG)linenum);
            (void)fprintf(stderr," and\n");
         }
         if (fake) (void)fprintf(stderr,"would have ");
         (void)fprintf(stderr,"had ");
         if (hex) plural("binary hex string",(LONG_LONG)repcount);
         else plural("string",(LONG_LONG)repcount);
         (void)fprintf(stderr," replaced");
         if (!this_binary)
         {
            (void)fprintf(stderr," in ");
            plural("line",(LONG_LONG)linecount);
         }
         (void)fprintf(stderr,"\n");
      }
   }
   return(gotfail);
}

#ifdef __STDC__
static int rename_file(char *source,char *dest)
#else
static int rename_file(source,dest)
char *source,*dest;
#endif
{
    /* Rename file source to file dest.
       Returns 0 for failure, 1 for success */
    if (rename(source,dest))
    {
       /* Note that the rename function CANNOT rename across filing systems,
          but /bin/mv CAN, so might as well try /bin/mv if rename fails.
          This is a weakness of "rpl -t" - if you set TMPDIR=/tmp, then
          try to replace strings in a file not on the same disk as /tmp
          using "rpl -t", you can't do it ! */
       char command[BUFSIZ];
       (void)snprintf(command,BUFSIZ,"%s \"%s\" \"%s\" 2>/dev/null",RENAME_COMMAND,source,dest);
       if (system(command)) return(0);
    }
    return(1);
}

#ifdef __STDC__
static int update_file(char *oldname,char *newname,char *orgname)
#else
static int update_file(oldname,newname,orgname)
char *oldname,*newname,*orgname;
#endif
{
   /* File needs to be updated and possibly a backup version kept */
   struct stat finfo;
   (void)stat(orgname,&finfo);
   if (rename_file(oldname,newname))
   {
      updated=1;
      if (verbose)
      {
         struct stat fst;
         LONG_LONG new_file_size;
         (void)stat(newname,&fst);
         new_file_size=(LONG_LONG)fst.st_size;
         (void)fprintf(stderr,"Updated %s %s (",filetype,newname);
         plural("byte",new_file_size);
         if (force) (void)fprintf(stderr,")\n");
         else (void)fprintf(stderr," - backup with %s extension)\n",backupsuff);
      }
      if (chmod(newname,finfo.st_mode))
        (void)fprintf(stderr,"WARNING: Couldn't set permissions of %s %s\n",filetype,newname);
      if (chown(newname,finfo.st_uid,finfo.st_gid))
        (void)fprintf(stderr,"WARNING: Couldn't set ownership of %s %s\n",filetype,newname);
      if (retainstamp)
      {
         struct utimbuf oldtime;
         oldtime.actime=finfo.st_atime;
         oldtime.modtime=finfo.st_mtime;
         if (utime(newname,&oldtime))
            (void)fprintf(stderr,"WARNING: Couldn't retain timestamp of %s %s\n",filetype,newname);
      }
      return(1);
   }
   else return(0);
}

#ifdef __STDC__
static void replace_a_file(const char *thename)
#else
static void replace_a_file(thename)
char *thename;
#endif
{
   /* Replace strings in a file, writing the new version to a temporary
      file which is either then deleted (no replacements) or renamed onto
      the original (some replacements) */
   FILE *filedes,*fileout=(FILE *)NULL;
   char repmess[BUFSIZ];

   (void)strcpy(intitle,thename);
   (void)snprintf(repmess,BUFSIZ,"strings in %s",intitle);
   if (!ask_user(1,repmess)) return;
   if ((filedes=fopen(intitle,"r"))==(FILE *)NULL)
        (void)fprintf(stderr,"WARNING: Skipped %s - could not open\n",
                intitle);
   else
   {
      char tmpname[BUFSIZ];
      int tmphandle=0;
      (void)snprintf(tmpname,BUFSIZ,"%s/replace_tmp_XXXXXX",tmpdir);
      if (!fake && (tmphandle=mkstemp(tmpname))==-1)
      {
         (void)fprintf(stderr,"WARNING: Skipped %s - couldn't create temp file %s\n",intitle,tmpname);
         (void)fclose(filedes);
      }
      else
      { 
         if (!fake) set_temp_file(tmpname);
         updated=0;
         if (!fake && (fileout=fdopen(tmphandle,"r+"))==(FILE *)NULL)
         {
            (void)fprintf(stderr,
                    "WARNING: Skipped %s - couldn't associate stream with temp file %s\n",
                    intitle,tmpname);
            (void)fclose(filedes);
            (void)close(tmphandle);
         }
         else
         {
            int gotfail;
            struct stat fst;
            LONG_LONG cur_file_size;

            (void)stat(intitle,&fst);
            cur_file_size=(LONG_LONG)fst.st_size;
            gotfail=read_lines(filedes,fileout,cur_file_size,intitle);
            if (!fake) gotfail|=fclose(fileout);
            gotfail|=fclose(filedes);
            if (gotfail)
               (void)fprintf(stderr,"WARNING: Read/write/close failure when accessing %s - skipped\n",intitle);
            else
            if (repcount)
            {
               int goodone=1;
               if (!fake)
               {
                  if (force) goodone=update_file(tmpname,intitle,intitle);
                  else
                  {
                     char outtitle[BUFSIZ];
                     (void)strcpy(outtitle,intitle);
                     (void)strcat(outtitle,backupsuff);
                     if ((goodone=rename_file(intitle,outtitle)))
                        goodone=update_file(tmpname,intitle,outtitle);
                  }
               }
               if (goodone) { numfilereps++; numreps+=repcount; }
               else (void)fprintf(stderr,"WARNING: Failed to update %s %s\n",filetype,intitle);
            }
         }
         if (!fake && !updated) (void)unlink(tmpname);
         if (!fake) set_temp_file("");
      }
   }
}

#ifdef __STDC__
static int do_branch(const char *fname,const struct stat *flstat,int fint)
#else
static int do_branch(fname,flstat,fint)
char *fname;
struct stat *flstat;
int fint;
#endif
{
   /* Walk down a directory tree, replacing strings in appropriately
      matching files */

#ifdef __STDC__
   const
#endif
   char *tmpfname=fname;
   int suffmatch=0;
   size_t flen,sloop;
   /* The next line is just to make the filename neater */
   if (!strncmp(tmpfname,"./",2)) { tmpfname+=2; }
   flen=strlen(tmpfname);
   switch (fint)
   {
      case FTW_F:
         if (flen<sufflen ||
             strncmp(&tmpfname[flen-sufflen],backupsuff,sufflen))
         {
            if (!suffixes) replace_a_file(tmpfname);
            else
            if (minsuff<=flen)
            for (sloop=1;sloop<=suffixes && !suffmatch;sloop++)
            {
               char *sptr=sufflist[sloop];
               size_t sxlen=strlen(sptr);
               if (!strncasecmp(&tmpfname[flen-sxlen],sptr,sxlen))
                 replace_a_file(tmpfname);
            }
         }
      break;
   }
   return(0);
}

#ifdef __STDC__
static void recurse_down(char *recdir)
#else
static void recurse_down(recdir)
char *recdir;
#endif
{
   /* Recursive replacements requested */
   if (verbose) (void)fprintf(stderr,"Recursing down %s dir tree\n",recdir);
   if (ftw(recdir,do_branch,MAX_DIR_LEVELS))
     (void)fprintf(stderr,"WARNING: Failed to recurse down %s dir tree\n",recdir);
}

static void finish_it(P(void))
{
   /* All replacements/files done, so display a summary if in verbose mode 2 */
   if (verbose==2 && numfiles>1)
   {
      (void)fprintf(stderr,"Number of files scanned: %d\n",numfiles);
      (void)fprintf(stderr,"Number of files that ");
      if (fake) (void)fprintf(stderr,"would have ");
      (void)fprintf(stderr,"had strings replaced: %d\n",numfilereps);
      (void)fprintf(stderr,"Number of ");
      if (fake) (void)fprintf(stderr,"potential ");
      (void)fprintf(stderr,"string replacements in total: %d\n",numreps);
   }
   tidy_up();
}

#ifdef __STDC__
static char *dirname(char *pname)
#else
static char *dirname(pname)
char *pname;
#endif
{
   /* Get the directory name of the passed filename path */
   static char dname[BUFSIZ],*dptr;
   (void)strcpy(dname,pname);
   if (!strcmp(pname,"/")) return("/");
   if ((dptr=strrchr(dname,'/'))!=(char *)NULL)
   {
      *dptr='\0'; 
      return(dname);
   } else return(".");
}

#ifdef __STDC__
char *basename_path(char *pname)
#else
char *basename_path(pname)
char *pname;
#endif
{
   /* Get the leafname of the passed filename path */
   char *dptr;
   if (!strcmp(pname,"/")) return("/");
   if ((dptr=strrchr(pname,'/'))!=(char *)NULL)
     return(&dptr[1]); else return(pname);
}

#ifdef __STDC__
static void do_the_file(char *fname,int sofar)
#else
static void do_the_file(fname,sofar)
char *fname;
int sofar;
#endif
{
   /* Run appropriate function for the filename passed. Note that this
      routine can be recursive (if followsoftlinks is on) */
   struct stat finfo;
   if (!lstat(fname,&finfo))
   {
      if (S_ISDIR(finfo.st_mode))
      {
         if (recursive) recurse_down(fname);
         else (void)fprintf(stderr,"WARNING: Skipped %s directory - use -r for recursion\n",fname);
      }
      else
      if (S_ISLNK(finfo.st_mode))
      {
         if (followsoftlinks)
         {
            char linkbuf[BUFSIZ];
            if (readlink(fname,linkbuf,BUFSIZ)<0)
              (void)fprintf(stderr,"WARNING: Can't read %s soft-link - skipped\n",fname);
            else
            if (sofar==MAX_SOFT_LINKS)
              (void)fprintf(stderr,"WARNING: Soft-link loop (%s) detected - skipped\n",fname);
            else
            if (linkbuf[0]=='/')
            do_the_file(linkbuf,sofar+1);
            else
            {
               char linkdest[BUFSIZ],fdir[BUFSIZ];
               (void)strcpy(fdir,dirname(fname));
               (void)snprintf(linkdest,BUFSIZ,"%s/%s",fdir,dirname(linkbuf));
               if (chdir(linkdest) || getcwd(linkdest,BUFSIZ)==(char *)NULL)
               {
                  (void)fprintf(stderr,"WARNING: Soft-link (%s) to non-existent dir - skipped\n",fname);
                  (void)chdir(curpwd);
               }
               else
               {
                  (void)chdir(curpwd);
                  (void)strcat(linkdest,"/");
                  (void)strcat(linkdest,basename_path(linkbuf));
                  do_the_file(linkdest,sofar+1);
               }
            }   
         }
         else (void)fprintf(stderr,"WARNING: Soft-link (%s) detected - skipped\n",fname);
      }
      else
      if (S_ISREG(finfo.st_mode))
      {
         if (finfo.st_size) replace_a_file(fname);
         else
         (void)fprintf(stderr,"WARNING: %s is zero-length - skipped\n",fname);
      }
      else (void)fprintf(stderr,"WARNING: %s is not a file/dir - skipped\n",fname);
   }
   else (void)fprintf(stderr,"WARNING: No such file/dir (%s) - skipped\n",fname);
}

void check_tmp_dir_exists(P(void))
{
   /* Make sure tmpdir is actually a directory - it's fatal if it isn't,
      because we can't do any replacements in files without it */
   struct stat tmpdirinfo;
   if (stat(tmpdir,&tmpdirinfo) || !S_ISDIR(tmpdirinfo.st_mode))
      leave("Temporary directory not found");
}

#ifdef __STDC__
int main(int argc,char **argv)
#else
int main(argc,argv)
int argc;
char **argv;
#endif
{
   /* Parse command line options and then either read from stdin/send to
      stdout, recurse down or do replacements on specified files */
   init_vars();
   get_options(argc,argv);
   if (optind==argc || !strcmp(argv[optind],"-"))
   {
      if (recursive)
      {
         check_tmp_dir_exists();
         recurse_down(".");
      }
      else
      {
         if (ask_user(1,"strings in standard input"))
         {
            (void)strcpy(intitle,"<stdin>");
            if (read_lines(stdin,stdout,(LONG_LONG)-1,intitle))
              (void)fprintf(stderr,"WARNING: Read/write error during stdin/stdout operations - aborted\n");
         }
      }
   }
   else
   {
      if (followsoftlinks)
      {
         if (getcwd(curpwd,BUFSIZ)==(char *)NULL)
            leave("Can't determine current working directory"); 
      }
      check_tmp_dir_exists();
      for (;optind<argc;optind++) do_the_file(argv[optind],0);
   }
   finish_it();
   return(0);
}
