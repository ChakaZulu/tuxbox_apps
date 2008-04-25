# Generated automatically from Makefile.in by configure.

prefix = /usr/local
bindir = ${exec_prefix}/bin

CC = powerpc-linux-gcc
LD = $(CC)
AS = $(CC)
INSTALL = /usr/bin/install -c

CFLAGS =  -O3 -DALLOW_UNALIGNED_IO  -fstrength-reduce -fthread-jumps  -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop  -fschedule-insns -fschedule-insns2 -fexpensive-optimizations  -fforce-mem -fforce-addr -fomit-frame-pointer
LDFLAGS = $(CFLAGS)  -s
ASFLAGS = $(CFLAGS)

TARGETS =  fbgnuboy

ASM_OBJS = asm/i386/cpu.o asm/i386/lcd.o asm/i386/refresh.s

SYS_DEFS = -DHAVE_CONFIG_H -DIS_BIG_ENDIAN -DIS_LINUX
SYS_OBJS = sys/nix/nix.o
SYS_INCS = -I/usr/local/include -I./sys/nix -I../../../driver/include/ 

FB_OBJS = sys/linux-dbox2/fbdev.o sys/linux-dbox2/kb.o sys/dbox2/keymap.o sys/linux-dbox2/joy.o sys/oss/oss.o
FB_LIBS = 

SVGA_OBJS = sys/svga/svgalib.o sys/pc/keymap.o sys/linux/joy.o sys/oss/oss.o
SVGA_LIBS = -L/usr/local/lib -lvga

SDL_OBJS = sys/sdl/sdl.o sys/sdl/keymap.o
SDL_LIBS = -L/usr/lib -lSDL -lpthread
SDL_CFLAGS = -I/usr/include/SDL -D_REENTRANT

X11_OBJS = sys/x11/xlib.o sys/x11/keymap.o sys/linux/joy.o sys/oss/oss.o
X11_LIBS = -L/usr/X11R6/lib -lX11 -lXext

all: $(TARGETS)

include Rules

fbgnuboy: $(OBJS) $(SYS_OBJS) $(FB_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(SYS_OBJS) $(FB_OBJS) -o $@ $(FB_LIBS)

sgnuboy: $(OBJS) $(SYS_OBJS) $(SVGA_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(SYS_OBJS) $(SVGA_OBJS) -o $@ $(SVGA_LIBS)

sdlgnuboy: $(OBJS) $(SYS_OBJS) $(SDL_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(SYS_OBJS) $(SDL_OBJS) -o $@ $(SDL_LIBS)

sys/sdl/sdl.o: sys/sdl/sdl.c
	$(MYCC) $(SDL_CFLAGS) -c $< -o $@

sys/sdl/keymap.o: sys/sdl/keymap.c
	$(MYCC) $(SDL_CFLAGS) -c $< -o $@

xgnuboy: $(OBJS) $(SYS_OBJS) $(X11_OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(SYS_OBJS) $(X11_OBJS) -o $@ $(X11_LIBS)

install: all
	$(INSTALL) -m 755 $(TARGETS) $(prefix)$(bindir)

clean:
	rm -f *gnuboy gmon.out *.o sys/*.o sys/*/*.o asm/*/*.o

distclean: clean
	rm -f config.* sys/nix/config.h Makefile




