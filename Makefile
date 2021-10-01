# This Makefile requires GNU make in case you didn't realise...

OS := $(shell uname -s)

SRCS = auto.c binary.c init.c replace.c text.c
OBJS = auto.o binary.o init.o replace.o text.o

PROGRAM = replace

TREE = /usr/local
BINDIR = $(TREE)/bin
MANTREE = $(TREE)/man
MANDIR = $(MANTREE)/man1

DEFINES = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
#COMMON_GCC_FLAGS = -O3 -Wall -Wshadow -Wpointer-arith -Wcast-qual -Waggregate-return -Wnested-externs -Wlong-long -Winline -DHAVE_LONG_LONG
COMMON_GCC_FLAGS = -O -I/usr/local/include -DHAVE_LONG_LONG 

ifeq ($(OS),Linux)

# Linux C compiler flags
CC = cc
CFLAGS_SOURCE = -D_GNU_SOURCE
# -D_BSD_SOURCE -D_POSIX_SOURCE
CFLAGS = $(COMMON_GCC_FLAGS) $(CFLAGS_SOURCE) 
LINT = splint
LINTFLAGS = $(DEFINES) $(CFLAGS_SOURCE) -D__linux__ -DHAVE_LONG_LONG +posixlib -unrecog -weak
PAGER = less

endif

ifeq ($(OS),HP-UX)

LINT = lint
LINTFLAGS = $(DEFINES) -Ae
PAGER = more


# HP-UX ANSI C compiler flags
CC = cc
CFLAGS = -O -Ae +w1 -z -DHAVE_LONG_LONG
LDFLAGS = $(CFLAGS)

ARCH := $(shell uname -m)
ifeq ($(ARCH),ia64)
CFLAGS += +DD64
endif

endif

CFLAGS += $(DEFINES)

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

install: $(PROGRAM) $(PROGRAM).1
	-if [ ! -d $(TREE) ]; then mkdir $(TREE); chmod a+rx $(TREE); fi
	-if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR); chmod a+rx $(BINDIR); fi
	-if [ ! -d $(MANTREE) ]; then mkdir $(MANTREE); chmod a+rx $(MANTREE); fi
	-if [ ! -d $(MANDIR) ]; then mkdir $(MANDIR); chmod a+rx $(MANDIR); fi
	-cp $(PROGRAM) $(BINDIR)
	-strip $(BINDIR)/$(PROGRAM)
	-cp $(PROGRAM).1 $(MANDIR)
	-chmod a+r $(MANDIR)/$(PROGRAM).1

binary.o: $(PROGRAM).h
init.o: $(PROGRAM).h
$(PROGRAM).o: $(PROGRAM).h
text.o: $(PROGRAM).h

test check: $(PROGRAM) tests/runtests
	@cd tests; ./runtests

verbosetest verbosecheck: $(PROGRAM) tests/runtests
	@cd tests; ./runtests 1

clean:
	/bin/rm -f $(OBJS) a.out $(PROGRAM) core *.cln tests/*.failed tests/test004.old

lint splint: $(SRCS)
	$(LINT) $(LINTFLAGS) $(SRCS) 2>&1 | $(PAGER)
