include ../config.mak
PSPSDK=$(shell psp-config --pspsdk-path)
PSPLIBSDIR=$(PSPSDK)/..

TARGET = PMP
OBJS = ffplay.o	\
	mytext.o \
	mystring.o \
	mydrawtext.o \
	mydrawbmp.o \
	mytextdata.o \
	myscrolbar.o \
	myjpeg.o \
	HanLib/han_lib.o \
	PspLib/pg.o \
	jonny/csc.o jonny/mem64.o jonny/mestub.o jonny/me.o jonny/me_csc.o

INCDIR = .. ../libavformat ../libavcodec ../libavutil ./jonny $(PSPLIBSDIR)/include $(PSPLIBSDIR)/include/freetype2 ../MyTTF
CFLAGS = -O2 -G0 -g -Wall -DHAVE_AV_CONFIG_H
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

USE_PSPSDK_LIBC =
LIBDIR = ..	$(PSPLIBSDIR)/lib
LDFLAGS =
LIBS = -ljpeg -lfreetype -lavformat -lavutil -lavcodec -logg -lz -lm -lpspgu -lpsppower -lpspaudiolib -lpspaudio

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP Media Player

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
