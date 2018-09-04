GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Os -Wall -W -marm -Wno-unused-parameter -I include
LDFLAGS = -L lib -lndls 
OBJCOPY := "$(shell which arm-elf-objcopy 2>/dev/null)"
ifeq (${OBJCOPY},"")
	OBJCOPY := arm-none-eabi-objcopy
endif
EXE = brickshooter-sdl
DISTDIR = .

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
vpath %.tns $(DISTDIR)
vpath %.elf $(DISTDIR)

all:	$(EXE).prg.tns

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $<

$(EXE).elf: $(OBJS)
	mkdir -p $(DISTDIR)
	$(LD) $^ -o $(DISTDIR)/$@ $(LDFLAGS)

$(EXE).tns: $(EXE).elf
	$(GENZEHN) --input $(DISTDIR)/$^ --output $(DISTDIR)/$@ $(ZEHNFLAGS)

$(EXE).prg.tns: $(EXE).tns
	make-prg $(DISTDIR)/$^ $(DISTDIR)/$@

clean:
	rm -f *.o *.elf
	rm -f $(DISTDIR)/$(EXE).tns
	rm -f $(DISTDIR)/$(EXE).elf
	rm -f $(DISTDIR)/$(EXE).prg.tns

