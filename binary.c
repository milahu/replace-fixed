/* binary.c - The binary-related code for the replace program.
   (C) Richard K. Lloyd 2001-2004
*/

#define Extern extern
#include "replace.h"

static int maxtimeswarn;
static size_t minloop,totbuff;
static LONG_LONG outpos;

#ifdef __STDC__
char *alloc_mem(char *theptr,size_t *cursize,size_t newsize)
#else
char *alloc_mem(theptr,cursize,newsize)
char *theptr;
size_t *cursize;
size_t newsize;
#endif
{
   /* Allocate newsize bytes of memory to a pointer that's already
      had cursize bytes allocated */
   if (*cursize<newsize)
   {
      if ((theptr=(char *)realloc((MALLOCPTR)theptr,newsize))==(char *)NULL)
         leave("Can't allocate memory for replacement operations");
      *cursize=newsize;
   }
   return(theptr);
}

#ifdef __STDC__
static char *out_string(char *thestr,size_t stlen)
#else
static char *out_string(thestr,stlen)
char *thestr;
size_t stlen;
#endif
{
   /* Return string (doesn't necessarily have a zero terminator)
      in either hex or ASCII */
   size_t hl;
   static char ostr[BUFSIZ];
   char hstr[4];
   if (hex)
   {
      ostr[0]='\0';
      for (hl=0;hl<stlen;hl++)
      {
         (void)snprintf(hstr,3,"%02x",(unsigned int)thestr[hl]);
         (void)strcat(ostr,hstr);
      }
      (void)strcat(ostr," hex");
   }
   else
   {
      (void)strcpy(ostr,"\""); hstr[1]='\0';
      for (hl=0;hl<stlen;hl++)
      if (thestr[hl]=='\0')
        (void)strcat(ostr,"\\0");
      else
      {
         hstr[0]=thestr[hl];
         (void)strcat(ostr,hstr);
      }
      (void)strcat(ostr,"\"");
   }
   return(ostr);
}

/*
   The algorithm for binary replacement is tricky because different
   length strings could straddle the binary chunk boundary of course.
   Here's the psuedo-code (something I hardly ever write
   because it's usually a waste of time, but this is quite complex
   and worth working out):

   B = binary buffer
   C = binary chunk size
   L = Chunk loading offset
   X = Old string array
   Y = New string array

   Alloc C bytes of RAM at &B[0]
   L=0
   repeat
      fread() in C-L bytes at &B[L]
      A = num bytes actually read in
      T = L+A   # i.e. total number of bytes in buffer
      if T>0 then
         minloop=C+1
         for eachstr in 1..numstrs
         do
            O=length of X[eachstr]
            if T>=O then
               loop=0
               while loop<=T-O
               do
                  if B[loop...loop+O-1]==X[eachstr]
                  then
                     replace string at B[loop] with Y[eachstr]
                     loop+=O
                  else
                     loop++
                  fi
               done
               if (loop<minloop) minloop=loop
            fi
         done
         if (minloop==C+1 && T<C)
            L=T
         else
            if (minloop<T)
            then
               fwrite() minloop bytes at &B[0]
               for copy in 0..T-minloop-1
               do
                   B[copy]=B[minloop+copy]
               done
               L=T-minloop
            else
               fwrite() T bytes at &B[0]
               L=0
            fi
         fi
      fi
   until A==0
   if (L>0) fwrite L bytes at &B[0]
*/

#ifdef __STDC__
static void replace_chunk(size_t eachstr)
#else
static void replace_chunk(eachstr)
size_t eachstr;
#endif
{
   /* Replace string number "eachstr" in the current binary chunk */
   size_t olen=oldstrlen[eachstr],nlen=newstrlen[eachstr];
   /* Note that we have to check the new string's len versus the
      old one here, cos auto-detect code may be calling the
      binary replace code without having compared the new length
      vs. the old length beforehand. Old string must also be >0 in length
      (auto-detect lets zero-length old strings through cos it's allowed for
      text files now) */
   if (totbuff>=olen && nlen<=olen && olen>0)
   {
      size_t loop=0;
      while (loop<=totbuff-olen)
      {
         int binmatch;
         size_t bl;
         if (sensitive)
            binmatch=!memcmp(&binchunkptr[loop],oldstr[eachstr],olen);
         else
         {
            binmatch=1;
            for (bl=0;bl<olen && binmatch;bl++)
            binmatch=(tolower(binchunkptr[loop+bl])==tolower(oldstr[eachstr][bl]));
         }

         if (binmatch)
         {
            if (maxtimes && repcount==maxtimes)
            {
               if (!maxtimeswarn)
               {
                  (void)fprintf(stderr,"WARNING: Attempt to exceed ");
                  plural("replacement",(LONG_LONG)maxtimes);
                  (void)fprintf(stderr," in binary file ignored\n");
                  maxtimeswarn=1;
               }
               loop++;
            }
            else
            {
               char oldtmp[BUFSIZ],obuf[BUFSIZ];
               LONG_LONG coffset=outpos+(LONG_LONG)loop;
               (void)snprintf(oldtmp,BUFSIZ,"%s with ",out_string(&binchunkptr[loop],olen));
               (void)strcat(oldtmp,out_string(newstr[eachstr],nlen));
               (void)snprintf(obuf,BUFSIZ," at offset " LONG_LONG_FORMAT,coffset);
               (void)strcat(oldtmp,obuf);
               if (ask_user(2,oldtmp))
               {
                  (void)memcpy(oldtmp,&binchunkptr[loop],olen);
                  if (sensitive)
                     (void)memcpy(&binchunkptr[loop],newstr[eachstr],nlen);
                  else
                  for (bl=0;bl<nlen;bl++)
                  {
                     char newchar=newstr[eachstr][bl];
                     if (isupper((int)binchunkptr[loop+bl]))
                        newchar=(char)toupper(newchar);
                     else newchar=(char)tolower(newchar);
                     binchunkptr[loop+bl]=newchar;
                  }

                  if (verbose==2)
                  {
                     (void)fprintf(stderr,"%s -> ",out_string(oldtmp,olen));
                     (void)fprintf(stderr,
                     " %s (offset: " LONG_LONG_FORMAT ")\n",
                     out_string(&binchunkptr[loop],nlen),coffset);
                  }
                  repcount++;
                  loop+=olen;
               } else loop++;
            }
         } else loop++;
      }
      if (loop<minloop) minloop=loop;
   }
}

#ifdef __STDC__
int binary_io(FILE *fd,FILE *fdout)
#else
int binary_io(fd,fdout)
FILE *fd,*fdout;
#endif
{
   /* Read/write from/to a binary file. Returns 0 for success, != 0 fails */
   int gotfail=0;
   size_t numread=1,loadoff=autobinsize;

   binchunkptr=alloc_mem(binchunkptr,&binchunksize,BIN_CHUNK);
   maxtimeswarn=0; outpos=0;

   while (numread>0 && !gotfail)
   {
      numread=fread((void *)&binchunkptr[loadoff],1,BIN_CHUNK-loadoff,fd);
      gotfail=ferror(fd);
      if (!gotfail && (totbuff=numread+loadoff)>0)
      {
         size_t estr;
         minloop=BIN_CHUNK+1;
         for (estr=1;estr<=numstrs;estr++) replace_chunk(estr);
         if (minloop==BIN_CHUNK+1 && totbuff<BIN_CHUNK)
            loadoff=totbuff;
         else
         if (minloop<totbuff)
         {
            outpos+=(LONG_LONG)minloop;
            if (!fake) (void)fwrite((void *)binchunkptr,minloop,1,fdout);
            for (estr=0;estr<=totbuff-minloop-1;estr++)
            binchunkptr[estr]=binchunkptr[minloop+estr];
            loadoff=totbuff-minloop;
         }
         else
         {
            outpos+=(LONG_LONG)totbuff;
            if (!fake) (void)fwrite((void *)binchunkptr,totbuff,1,fdout);
            loadoff=0;
         }
         if (!fake) gotfail=ferror(fdout);
      }
   }
   if (loadoff>0 && !gotfail)
   {
      outpos+=(LONG_LONG)loadoff;
      if (!fake)
      {
         (void)fwrite((void *)binchunkptr,loadoff,1,fdout);
         gotfail=ferror(fdout);
      }
   }
   return(gotfail);
}
