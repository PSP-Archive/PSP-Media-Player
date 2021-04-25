PSPSDK=$(shell psp-config --pspsdk-path)
PSPDIR=$(shell psp-config --psp-prefix)
TARGET_LIB = libogg.a
OBJS = src/bitwise.o src/framing.o

INCDIR = ./include
#CFLAGS = -O2 -G0 -DHAVE_CONFIG_H -D_WIN32 -D__MINGW32__
# Note: in order to compile w/o _WIN32 and __MINGW32__ the file include/ogg/config_types.h had to be modified
CFLAGS = -O2 -G0 -DHAVE_CONFIG_H


LIBDIR =
LDFLAGS =

include $(PSPSDK)/lib/build.mak

install: $(TARGET_LIB)
	@echo "Installing libogg into $(PSPDIR)"
	@mkdir -p $(PSPDIR)/include $(PSPDIR)/lib
	@cp include/ogg/ogg.h include/ogg/os_types.h include/ogg/config_types.h $(PSPDIR)/include/ogg
	@cp libogg.a  $(PSPDIR)/lib
	@echo "Done"
