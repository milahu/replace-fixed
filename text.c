/* text.c - Routines to handle replacements in text files
   (C) Richard K. Lloyd 2001-2004 */

#define Extern extern
#include "replace.h"

static int linereps,maxreplineswarn; /* Replacement counts/flags */
static size_t newlinepos; /* Where we've written to in the new line buffer */
static char *ostr,*nstr; /* Old and new strings */
static size_t osl,nsl; /* Old and new string sizes */
static size_t oldlen; /* Old string size */
static int asked; /* Asked about replacement for this string yet ? */

#ifdef __STDC__
static void copy_char(char thechar)
#else
static void copy_char(thechar)
char thechar;
#endif
{
   /* Copy a character into the new line buffer, allocating
      extra memory if necessary */
   if (newlinepos+1>=newlinesize)
      newline=alloc_mem(newline,&newlinesize,newlinesize*2+1);
   newline[newlinepos++]=thechar;
}

#ifdef __STDC__
static int display_replace(char *oldline,size_t linecol)
#else
static int display_replace(oldline,linecol)
char *oldline;
size_t linecol;
#endif
{
   /* Show replacement if in verbose mode level 2.
      Return 0 if to continue replacement or 1 if to abort */
   if (verbose==2)
   {
      char tmpold[BUFSIZ],tmpnew[BUFSIZ];
      (void)strncpy(tmpold,&oldline[linecol],osl); tmpold[osl]='\0';
      (void)strncpy(tmpnew,&newline[newlinepos-nsl],nsl); tmpnew[nsl]='\0';
      if (!asked)
      {
         char nicemess[BUFSIZ];
         (void)snprintf(nicemess,BUFSIZ,"\"%s\" with \"%s\" in line %d",ostr,nstr,linenum);
         (void)fprintf(stderr,"Line %d: %s\n",linenum,oldline);
         if (!ask_user(2,nicemess))
         {
            size_t retline;
            for (retline=0;retline<=oldlen;retline++)
            newline[retline]=oldline[retline];
            return(1);
         }
         asked=1;
      }
      (void)fprintf(stderr,"\"%s\" -> \"%s\" (line %d, column %d)\n",
                    tmpold,tmpnew,linenum,(int)(linecol+1));
   }
   repcount++;
   if (!linereps) linecount++;
   linereps++;
   return(0);
}

#ifdef __STDC__
static char *replace_line(char *oldline,size_t strnum)
#else
static char *replace_line(oldline,strnum)
char *oldline;
size_t strnum;
#endif
{
   /* Replace strings in "oldline" using old/new strings number "strnum" */
   size_t oldmax,a=0,comppos,c;
   char newchar;
   int maxtimeswarn=0;
                                                                                
   asked=(prompt!=2);
   ostr=oldstr[strnum],nstr=newstr[strnum];
   newlinepos=0; osl=oldstrlen[strnum]; nsl=newstrlen[strnum];
   oldlen=strlen(oldline); oldmax=oldlen-osl;

   if (linenum>=startline && osl<=oldlen)
   {
      if (oldlen && osl)
      {
         newline=alloc_mem(newline,&newlinesize,oldlen);
         while (a<startcol && a<oldlen) copy_char(oldline[a++]);
         while (a<=oldmax)
         {
            if (sensitive)
               c=(size_t)strncmp(&oldline[a],ostr,osl);
            else
               c=strncasecmp(&oldline[a],ostr,osl);
            if (word && !c)
            {
               if (a) c=isalnum((int)oldline[a-1]);
               if (!c && a<oldmax) c=isalnum((int)oldline[a+osl]);
            }
            if (c) copy_char(oldline[a++]);
            else
            if (maxtimes && linereps==maxtimes)
            {
               if (!maxtimeswarn)
               {
                  (void)fprintf(stderr,"WARNING: Attempt to exceed ");
                  plural("replacement",(LONG_LONG)maxtimes);
                  (void)fprintf(stderr," at line %d ignored\n",linenum);
                  maxtimeswarn=1;
               }
               copy_char(oldline[a++]);
            }
            else
            if (maxreplines && !linereps && linecount==maxreplines)
            {
               if (!maxreplineswarn)
               {
                  (void)fprintf(stderr,"WARNING: Attempt to exceed ");
                  plural("line replacement",(LONG_LONG)maxreplines);
                  (void)fprintf(stderr," in %s ignored\n",filetype);
                  maxreplineswarn=1;
               }
               copy_char(oldline[a++]);
            }
            else
            {
               comppos=a;
               for (c=0;c<nsl;c++)
               {
                  newchar=nstr[c];
                  if (!sensitive)
                  {
                     if (isupper((int)oldline[comppos++]))
                        newchar=(char)toupper(newchar);
                     else newchar=(char)tolower(newchar);
                     if (c>=osl-1) comppos--;
                  }
                  copy_char(newchar);
               }
               if (display_replace(oldline,a)) return(newline);
               a+=osl;
            }
         }
      }
      else
      /* If old string is "" and the line is also "", then copy the
         new string in. Otherwise, do nothing to line if it's "" */
      if (!oldlen && !osl)
      {
         for (c=0;c<nsl;c++) copy_char(nstr[c]);
         if (display_replace(oldline,a)) return(newline);
      }
   } 
   while (a<oldlen) copy_char(oldline[a++]);
   copy_char('\0');
   return(newline);
}

#ifdef __STDC__
static char *replace_fgets(char *str,size_t bufsize,FILE *stream)
#else
static char *replace_fgets(str,bufsize,stream)
char *str;
size_t bufsize;
FILE *stream;
#endif
{
   size_t bufpos=0;
   /* Routine that behaves like fgets(), but additionally will retrieve
      from 256-byte buffer used to determine file type. This is because
      you can't rewind(stdin) or fwrite() to stdin either to retrieve the
      stdin bytes. If the 256 bytes become exhausted during this routine,
      pre-pend what's read in and fgets() the additional bytes. If exhausted
      *before* the routine is called, just call fgets() directly. */
   if (autobinread<autobinsize)
   {
      int gotnl=0;
      while (autobinread<autobinsize && bufpos<bufsize-1 && !gotnl)
      {
         gotnl=(binchunkptr[autobinread]=='\n');
         str[bufpos++]=binchunkptr[autobinread++];
      }
      if (gotnl || bufpos==bufsize-1)
      {
         str[bufpos]='\0'; return(str);
      }
   }
   return(fgets(&str[bufpos],(int)(bufsize-bufpos),stream));
}

#ifdef __STDC__
int text_io(FILE *fd,FILE *fdout)
#else
int text_io(fd,fdout)
FILE *fd,*fdout;
#endif
{
   /* Read/write from/to a text file. Returns zero if success, != 0 if fails */
   int eof=0,gotfail=0;
   linereps=0; linecount=0; maxreplineswarn=0;
   thestring=alloc_mem(thestring,&thestringsize,BIN_CHUNK);
   while (!eof && !gotfail)
   {
      int eol=0,boff=0;
      char eolc,eolstr[2];
      eolstr[0]='\0';
      while (!eol && !eof && !gotfail)
      {
         thestring[boff]='\0';
         eof=(replace_fgets(&thestring[boff],thestringsize-boff,fd)==(char *)NULL && !thestring[boff]);
         gotfail=ferror(fd);
         if (!eof && !gotfail)
         {
            boff+=(strlen(&thestring[boff])-1);
            eolc=thestring[boff];
            if (eolc=='\n' || eolc=='\r')
            {
               thestring[boff]='\0'; eol=1;
               (void)strcpy(eolstr,"\n");
            }
            else
            {
               boff++;
               thestring=alloc_mem(thestring,&thestringsize,thestringsize+BIN_CHUNK);
            }
         }
      }
      if ((boff || eol) && !gotfail)
      {
         linenum++;
         if (numstrs==1)
         {
            if (fake) (void)replace_line(thestring,1);
            else (void)fprintf(fdout,"%s%s",replace_line(thestring,1),eolstr);
         }
         else
         {
            size_t eachstr;
            for (eachstr=1;eachstr<=numstrs;eachstr++)
            {
               char *tempstr=replace_line(thestring,eachstr);
               thestring=alloc_mem(thestring,&thestringsize,strlen(tempstr));
               (void)strcpy(thestring,tempstr);
            }
            if (!fake) (void)fprintf(fdout,"%s%s",thestring,eolstr);
         }
         linereps=0;
         if (!fake) gotfail=ferror(fdout);
      }
   }
   return(gotfail);
}
