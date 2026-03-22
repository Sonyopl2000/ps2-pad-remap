EE_BIN = ps2_pad_remap.elf
EE_OBJS = main.o
EE_LIBS = -lpad -lkernel -lc

all: $(EE_BIN)

clean:
        rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
