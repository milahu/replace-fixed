/* auto.c - The automatic binary vs. text detecting code
   (C) Richard K. Lloyd 2001-2004
*/

#define Extern extern
#include "replace.h"

/* Binary codes array - determines which of the 256 possible char
   values are binary bytes (would never appear in a text file). This is
   complicated by the existence of 8-bit chars of course - if people want
   to strongly argue that a char is text and not binary (or vice versa),
   e-mail replace@richardlloyd.org.uk and we'll discuss flipping that byte's
   status here in a future release. For the moment, I'm sticking to 7-bit
   viewable chars, plus selected 8-bit chars that annoying Word users drop
   happily into their "text" files when they convert them into text or HTML */

static int bincodes[256]=
/* 0 = It's a text char, 1 it's a binary char */
{
   /* Control codes first, most of which are binary */
   /* 000-007: */ 1, 1, 1, 1, 1, 1, 1, 1,
   /* 9 is a tab character, 10 is a line feed and 13 is carriage return */
   /* 008-015: */ 1, 0, 0, 1, 1, 0, 1, 1,
   /* 016-023: */ 1, 1, 1, 1, 1, 1, 1, 1,
   /* Idiotic DOS text files use CTRL-Z (26) to terminate ! */
   /* 024-031: */ 1, 1, 0, 1, 1, 1, 1, 1,
   /* We're now into text chars (32=space through to 126=tilde) */
   /* 032-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 040-039: */ 0, 0, 0, 0, 0, 0, 0, 0, 
   /* 048-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 056-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 064-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 072-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 080-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 088-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 096-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 104-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 112-039: */ 0, 0, 0, 0, 0, 0, 0, 0,
   /* 127 = delete */
   /* 120-127: */ 0, 0, 0, 0, 0, 0, 0, 1,
   /* 8-bit chars now - most are binary */
   /* 128 = Word space */
   /* 128-135: */ 0, 1, 1, 1, 1, 1, 1, 1, 
   /* 136-143: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 145 and 146 = Word apostrophe, 148 and 150 = Word dash */
   /* 144-151: */ 1, 0, 0, 1, 0, 1, 0, 1, 
   /* 152-159: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 163 = Pound sterling */
   /* 160-167: */ 1, 1, 1, 0, 1, 1, 1, 1, 
   /* 168-175: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 178 and 179 = Word double quote */
   /* 176-183: */ 1, 1, 0, 0, 1, 1, 1, 1, 
   /* 185 = Word apostrophe */
   /* 184-191: */ 1, 0, 1, 1, 1, 1, 1, 1, 
   /* 192-199: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 200-207: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 208-215: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 216-223: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 226 = Word space */
   /* 224-231: */ 1, 1, 0, 1, 1, 1, 1, 1, 
   /* 232-239: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 240-247: */ 1, 1, 1, 1, 1, 1, 1, 1, 
   /* 248-255: */ 1, 1, 1, 1, 1, 1, 1, 1
};

#ifdef __STDC__
int is_binary(FILE *fhand)
#else
int is_binary(fhand)
FILE *fhand;
#endif
{
   /* Given a freshly opened input file (with file pointer at start of
      file), determine if any of the first X bytes [X = length of file or
      256, whichever is the smaller] contain any binary codes. Return 1 if
      they do, otherwise return 0. If there's an error, issue a warning and
      return 0 for text.
   */
   int retval=0;
   if (autodetect)
   {
      binchunkptr=alloc_mem(binchunkptr,&binchunksize,MAX_BIN_BYTES);
      autobinsize=fread((void *)binchunkptr,1,MAX_BIN_BYTES,fhand);
      if (ferror(fhand))
         (void)fprintf(stderr,"WARNING: Input unreadable (assuming text data)\n");
      else
      if (autobinsize)
      {
         /* In theory, we could "rewind(fhand);" here to return the open
            file pointer back to the start of the file. Sadly, however, it
            doesn't work for stdin, so we have to re-use the binchunkptr
            buffer when we read the file for real. This is easy for binary
            reads (there's a start offset in the routine, so we set it to
            autobinsize), but tricky for the text reads (fgets() has to be
            simulated - see replace_fgets() in text.c) */
         size_t rloop;
         for (rloop=0;rloop<autobinsize && !retval;rloop++)
         if (bincodes[(unsigned char)binchunkptr[rloop]]) retval=1;
      }
   }
   else
   {
      retval=binary;
      autobinsize=0;
   }
   if (retval) (void)strcpy(filetype,"binary file");
   else (void)strcpy(filetype,"text file");
   autobinread=0; /* No bytes read from auto-detect buffer yet */
   return(retval);
}
